#!/usr/bin/env python3

import os
import sys
import yaml
from jinja2 import Environment, FileSystemLoader

def load_idl(path):
    with open(path) as f:
        return yaml.safe_load(f)

def render_template(env, template_name, context):
    template = env.get_template(template_name)
    return template.render(context)

def write_file(path, contents):
    with open(path, "w") as f:
        f.write(contents)

def method_id_name(interface, method_name):
    return f"METHOD_{interface.lower()}_{method_name}"

def main():
    if len(sys.argv) != 3:
        print("Usage: generator.py <input_yaml> <output_dir>")
        sys.exit(1)

    input_yaml = sys.argv[1]
    output_dir = sys.argv[2]

    print(f"[IDLGEN] Using Python: {sys.executable}")
    print(f"[IDLGEN] Loading IDL from: {input_yaml}")
    print(f"[IDLGEN] Writing output to: {output_dir}")

    idl = load_idl(input_yaml)
    interface = idl["interface"]
    methods = idl["methods"]

    context = {
        "interface": interface,
        "methods": methods
    }

    template_dir = os.path.join(os.path.dirname(__file__), "templates")
    env = Environment(
        loader=FileSystemLoader(template_dir),
        trim_blocks=True,
        lstrip_blocks=True,
    )

    env.filters["method_id_name"] = method_id_name

    os.makedirs(output_dir, exist_ok=True)

    write_file(os.path.join(output_dir, f"{interface.lower()}.h"),
               render_template(env, "header.j2", context))
    write_file(os.path.join(output_dir, f"{interface.lower()}_server.c"),
               render_template(env, "server.j2", context))
    write_file(os.path.join(output_dir, f"{interface.lower()}_client.c"),
               render_template(env, "client.j2", context))

if __name__ == "__main__":
    main()

