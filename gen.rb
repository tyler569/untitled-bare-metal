#!/usr/bin/env ruby

require 'yaml'

def format_param_list(params)
  params.map do |param|
    "#{param['type']} #{param['name']}"
  end.join(", ")
end

def format_call_list(params)
  params.map do |param|
    param['name']
  end.join(", ")
end

def format_type_only_list(params)
  params.map do |param|
    param['type']
  end.join(", ")
end

K_CAP_TYPE = 'cte_t *'

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

  def mr_parameters
    @parameters.filter do |param|
      param['type'] != 'cptr_t'
    end
  end

  def cap_parameters
    @parameters.filter do |param|
      param['type'] == 'cptr_t'
    end
  end

  def message_info
    "new_message_info (#{identifier_name}, 0, #{cap_parameters.length}, #{mr_parameters.length})"
  end

  def usermode_wrapper
    <<~EOF
    static inline int
    #{user_function_name} (#{format_param_list(user_param_list)}) {
      #{mr_parameters.map.with_index { |param, i| "set_mr (#{i}, (word_t)#{param['name']});" }.join("\n")}
      #{cap_parameters.map.with_index { |param, i| "set_cap (#{i}, #{param['name']});" }.join("\n")}
      message_info_t _info = #{message_info};
      message_info_t _result = _syscall2 (sys_call, obj, _info);
      return get_message_label (_result);
    }
    EOF
  end

  def kernel_unmarshal_and_call
    <<~EOF
    case #{identifier_name}: {
      #{mr_parameters.map.with_index do |param, i|
          "#{param['type']} #{param['name']} = (#{param['type']})get_mr (#{i});"
        end.join
      }
      #{cap_parameters.map { |param| "#{K_CAP_TYPE}#{param['name']};" }.join}

      dbg_printf ("#{user_function_name} ");

      if (cap_type (slot) != #{type.type_name})
        {
          err_printf ("invalid cap type: %s\\n", cap_type_string (cap_type (slot)));
          return return_ipc (illegal_operation, 0);
        }
      if (get_message_length (info) < #{mr_parameters.length})
        return return_ipc (truncated_message, 0);
      if (get_message_extra_caps (info) < #{cap_parameters.length})
        return return_ipc (truncated_message, 0);

      #{cap_parameters.map.with_index do |param, i|
        <<~EOS
        #{param['name']} = lookup_cap_slot_this_tcb (get_cap (#{i}), &error);
        if (error != no_error)
          {
            err_printf ("lookup_cap failed for cap #{i}\\n");
            set_mr (0, #{i});
            return return_ipc (error, 1);
          }
        EOS
      end.join}

      dbg_printf ("(#{kernel_print_format})\\n", #{kernel_print_param_list});

      return #{kernel_function_name} (#{format_call_list(kernel_param_list)});
      break;
    }
    EOF
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

type_objs = intf['types'].map do |type|
  KType.new(type['name'], type)
end

HEADERS = {
  global_constants: 'sys/syscall.h',
  user_methods: 'sys/user_method_stubs.h',
  kernel_methods: 'kern/methods.h',
  kernel_stubs: 'kern/kernel_method_stubs.c',
  syscall_dispatch: 'kern/syscall_dispatch.c'
}

File.open("include/#{HEADERS[:global_constants]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts '#pragma once'
  f.puts
  f.puts '#include "sys/types.h"'
  f.puts
  # type enum
  f.puts "enum object_type {"
  type_objs.each do |type|
    f.puts "  #{type.type_name},"
  end
  f.puts "  max_cap_type,"
  f.puts "};"
  f.puts
  f.puts "static inline const char *cap_type_string (word_t type) {"
  f.puts "  switch (type) {"
  type_objs.each do |type|
    f.puts "    case #{type.type_name}: return \"#{type.name}\";"
  end
  f.puts "    default: return \"unknown\";"
  f.puts "  }"
  f.puts "}"
  f.puts
  # method enum
  f.puts "enum method_id {"
  f.puts "  METHOD_invalid,"
  type_objs.each do |type|
    type.kmethods.each do |method|
      f.puts "  #{method.identifier_name},"
    end
  end
  f.puts "};"
  f.puts
  # error enum
  f.puts "enum error_code {"
  intf['errors'].each do |error|
    f.puts "  #{error},"
  end
  f.puts "  max_error_code,"
  f.puts "};"
  f.puts
  f.puts "static inline const char *error_string (error_t error) {"
  f.puts "  switch (error) {"
  intf['errors'].each do |error|
    f.puts "    case #{error}: return \"#{error.gsub('_', ' ')}\";"
  end
  f.puts "    default: return \"unknown\";"
  f.puts "  }"
  f.puts "}"
  f.puts
  # syscall number
  f.puts "enum syscall_number {"
  intf['system_calls'].each do |syscall|
    f.puts "  #{syscall},"
  end
  f.puts "};"
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
      f.puts "message_info_t #{method.kernel_function_name} (#{format_param_list(method.kernel_param_list)});"
    end
  end
end

File.open("include/#{HEADERS[:kernel_stubs]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts "#include \"#{HEADERS[:global_constants]}\""
  f.puts "#include \"#{HEADERS[:kernel_methods]}\""

  f.puts
  type_objs.each do |type|
    type.kmethods.each do |method|
      # puts kernel stub
      f.puts "__attribute__((weak)) message_info_t"
      f.puts "#{method.kernel_function_name} (#{format_type_only_list(method.kernel_param_list)}) {"
      f.puts "  err_printf(\"unimplemented kernel method #{method.kernel_function_name}\\n\");"
      f.puts "  return illegal_operation;"
      f.puts "}"
    end
  end
end

File.open("include/#{HEADERS[:syscall_dispatch]}", 'w') do |f|
  f.puts AUTOGENERATED_BANNER
  f.puts "message_info_t"
  f.puts "dispatch_method(#{K_CAP_TYPE} slot, message_info_t info) {"
  f.puts "error_t error = no_error;"
  f.puts "switch (get_message_label (info)) {"
  type_objs.each do |type|
    type.kmethods.each do |method|
      f.puts method.kernel_unmarshal_and_call
    end
  end
  f.puts "default:"
  f.puts "  return return_ipc (illegal_operation, 0);"
  f.puts "}"
  f.puts "}"
end

# format headers
`clang-format -i --style=GNU #{HEADERS.values.map { |v| "include/#{v}" }.join(' ')}`
