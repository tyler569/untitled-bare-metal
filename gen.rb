#!/usr/bin/env ruby

require 'yaml'

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
  f.puts "#{typename}_#{method['name']} (cptr_t obj, #{param_list(method)}) {"
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

def printf_format(method)
  method['parameters'].map do |param|
    if param['type'] == 'cptr_t'
      "#{param['name']}=cap:%s"
    else
      "#{param['name']}=%#lx"
    end
  end.join(", ")
end

def kernel_print(typename, method)
  "printf(\"#{typename}_#{method['name']}(service=cap:%s, #{printf_format(method)}\")\\n\", #{param_call_list(method)});"
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

output_usermode_wrappers($stdout, intf['types'])
output_type_enum($stdout, intf['types'])
output_method_enum($stdout, intf['types'])
output_kernel_switch($stdout, intf['types'])
