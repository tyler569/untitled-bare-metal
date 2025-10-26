#!/usr/bin/env ruby

require 'yaml'

K_CAP_TYPE = 'cte_t *'

# ----------------- Data Model -----------------

class Parameter
  attr_reader :name, :type

  def initialize(hash)
    @name = hash['name']
    @type = hash['type']
  end

  def capability?
    @type == 'cptr_t'
  end

  def message_register?
    !capability?
  end

  def to_declaration
    "#{@type} #{@name}"
  end
end

# ----------------- Marshalling -----------------

module Marshalling
  class ParameterTransformer
    def initialize(params)
      @params = params.map { |p| p.is_a?(Parameter) ? p : Parameter.new(p) }
    end

    def mr_parameters
      @params.select(&:message_register?)
    end

    def cap_parameters
      @params.select(&:capability?)
    end

    def format_param_list
      @params.map(&:to_declaration).join(", ")
    end

    def format_call_list
      @params.map(&:name).join(", ")
    end

    # Transform cptr_t -> cte_t* for kernel
    def kernel_transformed_params
      @params.map do |param|
        if param.capability?
          Parameter.new({'name' => param.name, 'type' => K_CAP_TYPE})
        else
          param
        end
      end
    end
  end

  class MessageInfo
    def initialize(method_identifier, params)
      @method_identifier = method_identifier
      @transformer = ParameterTransformer.new(params)
    end

    def to_c_expression
      "new_message_info (#{@method_identifier}, 0, #{@transformer.cap_parameters.length}, #{@transformer.mr_parameters.length})"
    end
  end
end

# ----------------- Code Generators -----------------

class UserStubGenerator
  def initialize(method)
    @method = method
    @transformer = Marshalling::ParameterTransformer.new(@method.user_param_list)
    @msg_info = Marshalling::MessageInfo.new(@method.identifier_name, @method.parameters)
  end

  def generate
    mr_params = @method.mr_parameters
    cap_params = @method.cap_parameters

    <<~EOF
    static inline int
    #{@method.user_function_name} (#{@transformer.format_param_list}) {
      #{mr_params.map.with_index { |param, i| "set_mr (#{i}, (word_t)#{param['name']});" }.join("\n")}
      #{cap_params.map.with_index { |param, i| "set_cap (#{i}, #{param['name']});" }.join("\n")}
      message_info_t _info = #{@msg_info.to_c_expression};
      __call_kernel (obj, _info);
      return get_message_label (__ipc_buffer->tag);
    }
    EOF
  end
end

class KernelDispatchGenerator
  def initialize(method)
    @method = method
  end

  def generate_parameter_length_check
    mr_params = @method.mr_parameters
    cap_params = @method.cap_parameters

    mr_check = if mr_params.length > 0
      <<~EOF
      do {
        word_t len = get_message_length (info);
        if (len < #{mr_params.length})
          return ipc_truncated_message(len, #{mr_params.length});
      } while (0);
      EOF
    else
      ""
    end

    cap_check = if cap_params.length > 0
      <<~EOF
      do {
        word_t len = get_message_extra_caps (info);
        if (len < #{cap_params.length})
          return ipc_truncated_message(len, #{cap_params.length});
      } while (0);
      EOF
    else
      ""
    end

    [mr_check, cap_check].reject(&:empty?).join("\n")
  end

  def generate_dispatch_case
    mr_params = @method.mr_parameters
    cap_params = @method.cap_parameters

    <<~EOF
    case #{@method.identifier_name}: {
      #{mr_params.map.with_index do |param, i|
          "#{param['type']} #{param['name']} = (#{param['type']})get_mr (#{i});"
        end.join
      }
      #{cap_params.map { |param| "#{K_CAP_TYPE}#{param['name']};" }.join}

      dbg_printf ("#{@method.user_function_name} ");

      if (cap_type (slot) != #{@method.type.type_name})
        {
          err_printf ("invalid cap type: %s\\n", cap_type_string (cap_type (slot)));
          return ipc_illegal_operation();
        }
      #{generate_parameter_length_check}

      #{cap_params.map.with_index do |param, i|
        <<~EOS
        error = lookup_cap_slot_this_tcb (get_cap (#{i}), &#{param['name']});
        if (error != no_error)
          {
            err_printf ("lookup_cap failed for cap #{i}\\n");
            set_mr (0, #{i});
            return return_ipc (error, 1);
          }
        EOS
      end.join}

      dbg_printf ("(#{@method.kernel_print_format})\\n", #{@method.kernel_print_param_list});

      return #{@method.kernel_function_name} (#{@method.kernel_call_list});
      break;
    }
    EOF
  end
end

class EnumGenerator
  def initialize(types, errors, syscalls)
    @types = types
    @errors = errors
    @syscalls = syscalls
  end

  def generate_object_type_enum
    lines = ["enum object_type {"]
    @types.each do |type|
      lines << "  #{type.type_name},"
    end
    lines << "  max_cap_type,"
    lines << "};"
    lines.join("\n")
  end

  def generate_cap_type_string_function
    lines = ["static inline const char *cap_type_string (word_t type) {"]
    lines << "  switch (type) {"
    @types.each do |type|
      lines << "    case #{type.type_name}: return \"#{type.name}\";"
    end
    lines << "    default: return \"unknown\";"
    lines << "  }"
    lines << "}"
    lines.join("\n")
  end

  def generate_method_id_enum
    lines = ["enum method_id {"]
    lines << "  METHOD_invalid,"
    @types.each do |type|
      type.kmethods.each do |method|
        lines << "  #{method.identifier_name},"
      end
    end
    lines << "};"
    lines.join("\n")
  end

  def generate_error_enum
    lines = ["enum error_code {"]
    @errors.each do |error|
      lines << "  #{error},"
    end
    lines << "  max_error_code,"
    lines << "};"
    lines.join("\n")
  end

  def generate_error_string_function
    lines = ["static inline const char *error_string (error_t error) {"]
    lines << "  switch (error) {"
    @errors.each do |error|
      lines << "    case #{error}: return \"#{error.gsub('_', ' ')}\";"
    end
    lines << "    default: return \"unknown\";"
    lines << "  }"
    lines << "}"
    lines.join("\n")
  end

  def generate_syscall_enum
    lines = ["enum syscall_number {"]
    @syscalls.each do |syscall|
      lines << "  #{syscall},"
    end
    lines << "};"
    lines.join("\n")
  end

  def generate_all
    [
      generate_object_type_enum,
      "",
      generate_cap_type_string_function,
      "",
      generate_method_id_enum,
      "",
      generate_error_enum,
      "",
      generate_error_string_function,
      "",
      generate_syscall_enum
    ].join("\n")
  end
end

class KMethod
  attr_reader :name, :parameters, :type

  def initialize(name, parameters, type)
    @name = name
    @parameters = parameters
    @type = type
  end

  def user_function_name
    @type.name + '_' + @name
  end

  def kernel_function_name
    @type.name + '_' + @name
  end

  def identifier_name
    'METHOD_' + @type.name + '_' + @name
  end

  def user_param_list
    [{'name'=>'obj', 'type'=>'cptr_t'}] + @parameters
  end

  def kernel_param_list
    [{'name'=>'slot', 'type'=>K_CAP_TYPE}] + @parameters.map do |param|
      if param['type'] == 'cptr_t'
        {'name'=>param['name'], 'type'=>K_CAP_TYPE}
      else
        param
      end
    end
  end

  def kernel_param_list_declaration
    kernel_param_list.map { |p| "#{p['type']} #{p['name']}" }.join(", ")
  end

  def kernel_call_list
    kernel_param_list.map { |p| p['name'] }.join(", ")
  end

  def kernel_print_param_list
    l = ['cap_type_string (slot)'] + @parameters.map do |param|
      if param['type'] == 'cptr_t'
        'cap_type_string (' + param['name'] + ')'
      else
        param['name']
      end
    end
    l.join(', ')
  end

  def kernel_print_format
    l = ['cap:%s']
    l += @parameters.map do |param|
      if param['type'] == 'cptr_t'
        "#{param['name']}=cap:%s"
      elsif param['type'] == 'uint8_t'
        "#{param['name']}=%hhu"
      elsif param['type'] == 'bool'
        "#{param['name']}=%d"
      elsif param['type'].include? '*'
        "#{param['name']}=%p"
      else
        "#{param['name']}=%#lx"
      end
    end
    l.join(', ')
  end

  def transformer
    @transformer ||= Marshalling::ParameterTransformer.new(@parameters)
  end

  def mr_parameters
    transformer.mr_parameters.map { |p| {'name' => p.name, 'type' => p.type} }
  end

  def cap_parameters
    transformer.cap_parameters.map { |p| {'name' => p.name, 'type' => p.type} }
  end

  def message_info
    Marshalling::MessageInfo.new(identifier_name, @parameters).to_c_expression
  end

  def usermode_wrapper
    UserStubGenerator.new(self).generate
  end

  def kernel_unmarshal_check_parameters_length
    KernelDispatchGenerator.new(self).generate_parameter_length_check
  end

  def kernel_unmarshal_and_call
    KernelDispatchGenerator.new(self).generate_dispatch_case
  end
end

class KType
  attr_reader :name, :kmethods

  def initialize(name, yaml)
    @name = name

    @kmethods = yaml['methods'].map do |method|
      KMethod.new(method['name'], method['parameters'], self)
    end
  end

  def type_name
    'cap_' + @name
  end
end

# ----------------- Main -----------------

AUTOGENERATED_BANNER = <<~EOF
// This file is autogenerated by '#{$0}'. Do not edit.

EOF

intf = YAML.load_file('interface.yml')
errors = YAML.load_file('errors.yml')
syscalls = YAML.load_file('syscalls.yml')

type_objs = intf['types'].map do |type|
  KType.new(type['name'], type)
end

HEADERS = {
  global_constants: 'sys/syscall.h',
  user_methods: 'sys/user_method_stubs.h',
  kernel_methods: 'kern/methods.h',
  syscall_dispatch: 'kern/syscall_dispatch.c'
}

enum_generator = EnumGenerator.new(type_objs, errors, syscalls)

File.open("include/#{HEADERS[:global_constants]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts '#pragma once'
  f.puts
  f.puts '#include "sys/types.h"'
  f.puts
  f.puts enum_generator.generate_all
end

File.open("include/#{HEADERS[:user_methods]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts '#pragma once'
  f.puts
  f.puts "#include \"#{HEADERS[:global_constants]}\""
  f.puts
  f.puts "static inline word_t get_mr (word_t index);"
  f.puts "static inline void set_mr (word_t index, word_t value);"
  f.puts "static inline cptr_t get_cap (word_t index);"
  f.puts "static inline void set_cap (word_t index, cptr_t value);"
  f.puts
  type_objs.each do |type|
    type.kmethods.each do |method|
      f.puts method.usermode_wrapper
    end
  end

end

File.open("include/#{HEADERS[:kernel_methods]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts '#pragma once'
  f.puts
  f.puts "#include \"#{HEADERS[:global_constants]}\""
  f.puts "#include \"kern/cap.h\""
  f.puts
  type_objs.each do |type|
    type.kmethods.each do |method|
      # puts kernel header
      f.puts "error_t #{method.kernel_function_name} (#{method.kernel_param_list_declaration});"
    end
  end
end

File.open("include/#{HEADERS[:syscall_dispatch]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts "error_t"
  f.puts "dispatch_method(#{K_CAP_TYPE} slot, message_info_t info) {"
  f.puts "error_t error = no_error;"
  f.puts "switch (get_message_label (info)) {"
  type_objs.each do |type|
    type.kmethods.each do |method|
      f.puts method.kernel_unmarshal_and_call
    end
  end
  f.puts "default:"
  f.puts "  return ipc_illegal_operation();"
  f.puts "}"
  f.puts "}"
end

# format headers
`clang-format -i --style=GNU #{HEADERS.values.map { |v| "include/#{v}" }.join(' ')}`
