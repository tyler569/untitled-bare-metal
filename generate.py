#!/usr/bin/env python3
import os
import subprocess
import yaml
from jinja2 import Environment, BaseLoader

# --- Helper Functions ---

def format_param_list(params):
    return ", ".join(f"{p['type']} {p['name']}" for p in params)

def format_call_list(params):
    return ", ".join(p['name'] for p in params)

def format_type_only_list(params):
    return ", ".join(p['type'] for p in params)

K_CAP_TYPE = "cte_t *"

# --- Classes ---

class KMethod:
    def __init__(self, name, parameters, ktype):
        self.name = name
        self.parameters = parameters
        self.ktype = ktype

    @property
    def user_function_name(self):
        return f"{self.ktype.name}_{self.name}"

    @property
    def kernel_function_name(self):
        return f"{self.ktype.name}_{self.name}"

    @property
    def identifier_name(self):
        return f"METHOD_{self.ktype.name}_{self.name}"

    @property
    def user_param_list(self):
        # Prepend an object parameter.
        return [{'name': 'obj', 'type': 'cptr_t'}] + self.parameters

    @property
    def kernel_param_list(self):
        # Prepend the 'slot' parameter, and convert any cptr_t to K_CAP_TYPE.
        new_params = []
        for p in self.parameters:
            if p['type'] == 'cptr_t':
                new_params.append({'name': p['name'], 'type': K_CAP_TYPE})
            else:
                new_params.append(p)
        return [{'name': 'slot', 'type': K_CAP_TYPE}] + new_params

    @property
    def kernel_print_param_list(self):
        lst = ["cap_type_string (slot)"]
        for p in self.parameters:
            if p['type'] == 'cptr_t':
                lst.append(f"cap_type_string ({p['name']})")
            else:
                lst.append(p['name'])
        return ", ".join(lst)

    @property
    def kernel_print_format(self):
        lst = ["cap:%s"]
        for p in self.parameters:
            t = p['type']
            if t == 'cptr_t':
                lst.append(f"{p['name']}=cap:%s")
            elif t == 'uint8_t':
                lst.append(f"{p['name']}=%hhu")
            elif t == 'bool':
                lst.append(f"{p['name']}=%d")
            elif '*' in t:
                lst.append(f"{p['name']}=%p")
            else:
                lst.append(f"{p['name']}=%#lx")
        return ", ".join(lst)

    @property
    def mr_parameters(self):
        return [p for p in self.parameters if p['type'] != 'cptr_t']

    @property
    def cap_parameters(self):
        return [p for p in self.parameters if p['type'] == 'cptr_t']

    @property
    def message_info(self):
        return f"new_message_info ({self.identifier_name}, 0, {len(self.cap_parameters)}, {len(self.mr_parameters)})"

    @property
    def usermode_wrapper(self):
        mr_lines = "\n  ".join(
            f"set_mr ({i}, (word_t){p['name']});" 
            for i, p in enumerate(self.mr_parameters)
        )
        cap_lines = "\n  ".join(
            f"set_cap ({i}, {p['name']});"
            for i, p in enumerate(self.cap_parameters)
        )
        return f"""static inline int
{self.user_function_name} ({format_param_list(self.user_param_list)}) {{
  {mr_lines}
  {cap_lines}
  message_info_t _info = {self.message_info};
  message_info_t _result = _syscall2 (sys_call, obj, _info);
  return get_message_label (_result);
}}
"""

    @property
    def kernel_unmarshal_and_call(self):
        mr_lines = "".join(
            f"{p['type']} {p['name']} = ({p['type']})get_mr ({i});\n      "
            for i, p in enumerate(self.mr_parameters)
        )
        cap_decl_lines = "".join(
            f"{K_CAP_TYPE}{p['name']};\n      " for p in self.cap_parameters
        )
        cap_lookup_lines = "".join(
            f"""{p['name']} = lookup_cap_slot_this_tcb (get_cap ({i}), &error);
      if (error != no_error)
        {{
          err_printf ("lookup_cap failed for cap {i}\\n");
          set_mr (0, {i});
          return return_ipc (error, 1);
        }}
      """
            for i, p in enumerate(self.cap_parameters)
        )
        return f"""case {self.identifier_name}: {{
      {mr_lines}{cap_decl_lines}
      dbg_printf ("{self.user_function_name} ");
      
      if (cap_type (slot) != {self.ktype.type_name})
        {{
          err_printf ("invalid cap type: %s\\n", cap_type_string (cap_type (slot)));
          return return_ipc (illegal_operation, 0);
        }}
      if (get_message_length (info) < {len(self.mr_parameters)})
        return return_ipc (truncated_message, 0);
      if (get_message_extra_caps (info) < {len(self.cap_parameters)})
        return return_ipc (truncated_message, 0);
      
      {cap_lookup_lines}
      dbg_printf ("({self.kernel_print_format})\\n", {self.kernel_print_param_list});
      
      return {self.kernel_function_name} ({format_call_list(self.kernel_param_list)});
      break;
    }}
"""

class KType:
    def __init__(self, name, data):
        self.name = name
        self.kmethods = [KMethod(m['name'], m['parameters'], self)
                         for m in data.get('methods', [])]

    @property
    def type_name(self):
        return f"cap_{self.name}"

# --- Jinja2 Templates ---

AUTOGENERATED_BANNER = "// This file is autogenerated by '{}'. Do not edit.\n".format(os.path.basename(__file__))

global_constants_template = r'''{{ banner }}#pragma once

#include "sys/types.h"

enum object_type {
{% for t in types %}
  {{ t.type_name }},
{% endfor %}
  max_cap_type,
};

static inline const char *cap_type_string (word_t type) {
  switch (type) {
{% for t in types %}
    case {{ t.type_name }}: return "{{ t.name }}";
{% endfor %}
    default: return "unknown";
  }
}

enum method_id {
  METHOD_invalid,
{% for t in types %}
  {% for m in t.kmethods %}
  {{ m.identifier_name }},
  {% endfor %}
{% endfor %}
};

enum error_code {
{% for err in errors %}
  {{ err }},
{% endfor %}
  max_error_code,
};

static inline const char *error_string (error_t error) {
  switch (error) {
{% for err in errors %}
    case {{ err }}: return "{{ err.replace('_', ' ') }}";
{% endfor %}
    default: return "unknown";
  }
}

enum syscall_number {
{% for sc in system_calls %}
  {{ sc }},
{% endfor %}
};
'''

user_methods_template = r'''{{ banner }}#pragma once

#include "{{ global_constants }}"
static inline word_t get_mr (word_t index);
static inline void set_mr (word_t index, word_t value);
static inline cptr_t get_cap (word_t index);
static inline void set_cap (word_t index, cptr_t value);

{% for t in types %}
{% for m in t.kmethods %}
{{ m.usermode_wrapper }}
{% endfor %}
{% endfor %}
'''

kernel_methods_template = r'''{{ banner }}#pragma once

#include "{{ global_constants }}"
#include "kern/cap.h"
{% for t in types %}
{% for m in t.kmethods %}
message_info_t {{ m.kernel_function_name }} ({{ format_param_list(m.kernel_param_list) }});
{% endfor %}
{% endfor %}
'''

kernel_stubs_template = r'''{{ banner }}
#include "{{ global_constants }}"
#include "{{ kernel_methods }}"

{% for t in types %}
{% for m in t.kmethods %}
__attribute__((weak)) message_info_t
{{ m.kernel_function_name }} ({{ format_type_only_list(m.kernel_param_list) }}) {
  err_printf("unimplemented kernel method {{ m.kernel_function_name }}\n");
  return illegal_operation;
}
{% endfor %}
{% endfor %}
'''

syscall_dispatch_template = r'''{{ banner }}
message_info_t
dispatch_method({{ K_CAP_TYPE }} slot, message_info_t info) {
  error_t error = no_error;
  switch (get_message_label (info)) {
{% for t in types %}
{% for m in t.kmethods %}
{{ m.kernel_unmarshal_and_call }}
{% endfor %}
{% endfor %}
    default:
      return return_ipc (illegal_operation, 0);
  }
}
'''

# --- Main Generation Function ---

def main():
    # Load YAML interface.
    with open("interface.yml") as f:
        intf = yaml.safe_load(f)

    type_objs = [KType(t['name'], t) for t in intf.get('types', [])]

    # Prepare a Jinja2 environment. We add our helper functions as globals.
    env = Environment(loader=BaseLoader())
    env.globals.update({
        'format_param_list': format_param_list,
        'format_type_only_list': format_type_only_list,
        'K_CAP_TYPE': K_CAP_TYPE,
    })

    # Data to pass to templates.
    template_data = {
        "banner": AUTOGENERATED_BANNER,
        "types": type_objs,
        "errors": intf.get("errors", []),
        "system_calls": intf.get("system_calls", []),
        "global_constants": "sys/syscall.h",  # file names used in include directives
        "kernel_methods": "kern/methods.h",
    }

    # Map header keys to template strings and output file paths.
    outputs = {
        os.path.join("include", "sys/syscall.h"): global_constants_template,
        os.path.join("include", "sys/user_method_stubs.h"): user_methods_template,
        os.path.join("include", "kern/methods.h"): kernel_methods_template,
        os.path.join("include", "kern/kernel_method_stubs.c"): kernel_stubs_template,
        os.path.join("include", "kern/syscall_dispatch.c"): syscall_dispatch_template,
    }

    # Ensure the include directory exists.
    os.makedirs("include", exist_ok=True)

    # Render and write each output file.
    for path, template_str in outputs.items():
        template = env.from_string(template_str)
        rendered = template.render(**template_data)
        with open(path, "w") as f:
            f.write(rendered)

    # Run clang-format on the generated files.
    header_files = " ".join(os.path.join("include", p) for p in [
        "sys/syscall.h", "sys/user_method_stubs.h",
        "kern/methods.h", "kern/kernel_method_stubs.c",
        "kern/syscall_dispatch.c"
    ])
    subprocess.run(f"clang-format -i {header_files}", shell=True, check=True)

if __name__ == "__main__":
    main()
