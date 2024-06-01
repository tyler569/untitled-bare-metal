#!/usr/bin/env ruby

require 'yaml'

def format_param_list(params)
  params.map do |param|
    "#{param['type']} #{param['name']}"
  end.join(", ")
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
    [{'name'=>'obj', 'type'=>'cap_t'}] + @parameters.map do |param|
      if param['type'] == 'cptr_t'
        {'name'=>param['name'], 'type'=>'cap_t'}
      else
        param
      end
    end
  end

  def kernel_print_param_list
    l = ['cap_type_string (service)'] + @parameters.map do |param|
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
      #{mr_parameters.map.with_index { |param, i| "set_mr (#{i}, #{param['name']});" }.join("\n")}
      #{cap_parameters.map.with_index { |param, i| "set_cap (#{i}, #{param['name']});" }.join("\n")}
      message_info_t info = #{message_info};
      return _syscall2 (sys_call, obj, info);
    }
    EOF
  end

  def kernel_unmarshal_and_call
    <<~EOF
    case #{identifier_name}: {
      #{mr_parameters.map.with_index { |param, i| "#{param['type']} #{param['name']} = get_mr (#{i});" }.join}
      #{cap_parameters.map { |param| "cap_t #{param['name']};" }.join}
      error = lookup_cptr (obj, &service);
      RETURN_IPC_LOOKUP_ERROR_UNLESS_OK (error, -1);

      if (cap_type (service) != #{type.type_name})
        RETURN_IPC_ERROR (illegal_operation);
      if (get_reg_count (info) <= #{mr_parameters.length})
        RETURN_IPC_ERROR (truncated_message);
      if (get_cap_count (info) <= #{cap_parameters.length})
        RETURN_IPC_ERROR (truncated_message);
      #{cap_parameters.map.with_index do |param, i|
        <<~EOS
        error = lookup_cap (#{i}, &#{param['name']});
        RETURN_IPC_LOOKUP_ERROR_UNLESS_OK (error, #{i});
        EOS
      end.join}
      printf("#{user_function_name}(#{kernel_print_format})\\n", #{kernel_print_param_list});
      #{kernel_function_name} (#{format_param_list(kernel_param_list)});
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

class Syscall
end

class Error
end

def param_list(method)
  l = ["cptr_t obj"] + method['parameters'].map do |param|
    "#{param['type']} #{param['name']}"
  end
  l.join(", ")
end

def param_call_list(method)
  l = ["service"] + method['parameters'].map do |param|
    param['name']
  end
  l.join(", ")
end

def printf_param_call_list(method)
  l = ["cap_type_string (service)"] + method['parameters'].map do |param|
    if param['type'] == 'cptr_t'
      "cap_type_string (#{param['name']})"
    else
      "#{param['name']}"
    end
  end
  l.join(", ")
end

def mr_params(method)
  method['parameters'].filter do |param|
    param['type'] != 'cptr_t'
  end
end

def cap_params(method)
  method['parameters'].filter do |param|
    param['type'] == 'cptr_t'
  end
end

def param_set_mr(param, i)
  "set_mr (#{i}, #{param['name']});"
end

def param_set_cap(param, i)
  "set_cap (#{i}, #{param['name']});"
end

def output_usermode_wrapper(f, typename, method)
  mrs = mr_params(method)
  caps = cap_params(method)

  f.puts "static inline int"
  f.puts "#{typename}_#{method['name']} (#{param_list(method)}) {"
  mrs.each_with_index do |param, i|
    f.puts "  #{param_set_mr(param, i)}"
  end
  caps.each_with_index do |param, i|
    f.puts "  #{param_set_cap(param, i)}"
  end
  f.puts "  message_info_t info = new_message_info (METHOD_#{typename}_#{method['name']}, 0, #{caps.length}, #{mrs.length});"
  f.puts "  return _syscall2 (sys_call, obj, info);"
  f.puts "}"
end

def output_usermode_wrappers(f, types)
  f.puts <<~EOF
  #include "sys/types.h"
  #include "sys/syscall.h"

  static inline int
  _syscall2 (int syscall, cptr_t obj, message_info_t info)
  {
    asm volatile ("syscall"
    : "+a" (syscall)
    : "D" (obj), "S" (info)
    : "memory");

    return syscall;
  }

  EOF

  types.each do |type|
    type['methods'].each do |method|
      output_usermode_wrapper(f, type['name'], method)
    end
  end
end

def output_type_enum(f, types)
  f.puts "enum object_types {"
  types.each do |type|
    f.puts "  cap_#{type['name']},"
  end
  f.puts "};"
end

def output_type_names(f, types)
  f.puts "const char *object_type_names[] = {"
  types.each do |type|
    f.puts "  [cap_#{type['name']} = \"#{type['name']}\","
  end
  f.puts "};"
end

def output_method_enum(f, types)
  f.puts "enum methods {"
  types.each do |type|
    type['methods'].each do |method|
      f.puts "  METHOD_#{type['name']}_#{method['name']},"
    end
  end
  f.puts "};"
end

def output_method_names(f, types)
  f.puts "const char *method_names[] = {"
  types.each do |type|
    type['methods'].each do |method|
      f.puts "  [METHOD_#{type['name']}_#{method['name']}] = \"#{type['name']}_#{method['name']}\","
    end
  end
  f.puts "};"
end

def output_syscall_enum(f, syscalls)
  f.puts "enum syscalls {"
  syscalls.each do |syscall|
    f.puts "  SYSCALL_#{syscall},"
  end
  f.puts "};"
end

def output_syscall_names(f, syscalls)
  f.puts "const char *syscall_names[] = {"
  syscalls.each do |syscall|
    f.puts "  [SYSCALL_#{syscall}] = \"#{syscall}\","
  end
  f.puts "};"
end

def output_error_enum(f, errors)
  f.puts "enum errors {"
  errors.each do |error|
    f.puts "  #{error},"
  end
  f.puts "};"
end

def output_error_names(f, errors)
  f.puts "const char *error_names[] = {"
  errors.each do |error|
    f.puts "  [#{error}] = \"#{error}\","
  end
  f.puts "};"
end

def printf_format(method)
  l = ['cap:%s']
  l += method['parameters'].map do |param|
    if param['type'] == 'cptr_t'
      "#{param['name']}=cap:%s"
    else
      "#{param['name']}=%#lx"
    end
  end
  l.join(', ')
end

def kernel_print(typename, method)
  "printf(\"#{typename}_#{method['name']}(#{printf_format(method)}\")\\n\", #{printf_param_call_list(method)});"
end

def output_kernel_switch(f, types)
  types.each do |type|
    f.puts "switch (method) {"
    type['methods'].each do |method|
      mrs = mr_params(method)
      caps = cap_params(method)

      f.puts "case METHOD_#{type['name']}_#{method['name']}: {"
      f.puts "  exception_t error = no_error;"
      f.puts "  cap_t service;"
      mrs.each_with_index do |param, i|
        f.puts "  #{param['type']} #{param['name']} = get_mr (#{i});"
      end
      caps.each_with_index do |param, i|
        f.puts "  cap_t #{param['name']};"
      end
      f.puts "  error = lookup_cptr (obj, &service);"
      f.puts "  RETURN_IPC_ERROR_UNLESS_OK (error, -1);"
      caps.each_with_index do |param, i|
        f.puts "  error = lookup_cap (#{i}, &#{param['name']});"
        f.puts "  RETURN_IPC_ERROR_UNLESS_OK (error, #{i});"
      end
      f.puts "  #{kernel_print(type['name'], method)}"
      f.puts "  return #{type['name']}_#{method['name']} (#{param_call_list(method)});"
      f.puts "}"
    end
    f.puts "default:"
    f.puts "  RETURN_IPC_ERROR (illegal_operation);"
    f.puts "}"
  end
end

intf = YAML.load_file('interface.yml')

# output_usermode_wrappers($stdout, intf['types'])
# output_type_enum($stdout, intf['types'])
# output_method_enum($stdout, intf['types'])
# output_kernel_switch($stdout, intf['types'])
# output_syscall_enum($stdout, intf['system_calls'])
# output_syscall_names($stdout, intf['system_calls'])
# output_error_enum($stdout, intf['errors'])
# output_error_names($stdout, intf['errors'])

type_objs = intf['types'].map do |type|
  KType.new(type['name'], type)
end

type_objs.each do |type|
  type.kmethods.each do |method|
    puts method.usermode_wrapper
  end
end

puts "int main() {"
puts "exception_t error = no_error;"
puts "cap_t obj;"
puts "switch (method) {"
type_objs.each do |type|
  type.kmethods.each do |method|
    puts method.kernel_unmarshal_and_call
  end
end
puts "default:"
puts "  RETURN_IPC_ERROR (illegal_operation);"
puts "}"
puts "}"

# method enum
puts "enum methods {"
puts "  METHOD_invalid,"
type_objs.each do |type|
  type.kmethods.each do |method|
    puts "  #{method.identifier_name},"
  end
end
puts "};"

