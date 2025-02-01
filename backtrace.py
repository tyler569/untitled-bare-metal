#!/usr/bin/env python3
import sys
import os
import re
import argparse
import subprocess
from itertools import groupby

def run_addr2line(addr2line_binary, target_file, addresses):
    """Call llvm-addr2line with the provided addresses."""
    if not addresses:
        return
    cmd = [addr2line_binary, '-fips', '-e', target_file] + addresses
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error calling {addr2line_binary}: {e}", file=sys.stderr)
        sys.exit(1)
    return result.stdout

def print_uniq_c(output):
    """
    Simulate `uniq -c` on the output (grouping consecutive identical lines).
    """
    lines = output.splitlines()
    for line, group in groupby(lines):
        count = sum(1 for _ in group)
        print(f"{count} {line}")

def main():
    parser = argparse.ArgumentParser(
        description="Process 'last_output' and use llvm-addr2line to resolve addresses.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        'file',
        nargs='?',
        default="build/kern/untitled_bare_metal",
        help="The target file for llvm-addr2line"
    )
    args = parser.parse_args()
    target_file = args.file

    if not os.path.isfile(target_file):
        print(f"File not found: {target_file}", file=sys.stderr)
        print(f"Usage: {sys.argv[0]} [file]", file=sys.stderr)
        sys.exit(1)

    addr2line_binary = "llvm-addr2line"

    # Read the last_output file
    last_output_file = "last_output"
    if not os.path.isfile(last_output_file):
        print(f"File not found: {last_output_file}", file=sys.stderr)
        sys.exit(1)

    with open(last_output_file, "r") as f:
        lines = f.readlines()

    # --- Case 1: pattern: (0x... ) <...>
    # The sed command in Bash:
    #   sed 's/.*(0x\(.*\)) .*/\1/g'
    # extracts the content between "0x" and ") " from matching lines.
    pattern1 = re.compile(r'\(0x(.*?)\) <.*>')
    if any(pattern1.search(line) for line in lines):
        addresses = []
        for line in lines:
            m = pattern1.search(line)
            if m:
                addr = m.group(1)
                addresses.append(addr)
        output = run_addr2line(addr2line_binary, target_file, addresses)
        if output:
            print(output, end='')
        return

    # --- Case 2: pattern: lines starting with spaces then a number and a colon, containing "IP="
    # In Bash:
    #   awk '{print $7}' | cut -c4-
    # extracts the 7th whitespace‐delimited field and removes its first three characters.
    pattern2 = re.compile(r'^\s+\d+:.*IP=')
    if any(pattern2.search(line) for line in lines):
        addresses = []
        for line in lines:
            if pattern2.search(line):
                parts = line.split()
                if len(parts) >= 7:
                    # awk prints field 7 (index 6), then cut -c4- (drop first 3 characters)
                    addr_field = parts[6]
                    addr = addr_field[3:] if len(addr_field) > 3 else ""
                    if addr:
                        addresses.append(addr)
        output = run_addr2line(addr2line_binary, target_file, addresses)
        if output:
            # Bash uses "uniq -c" on the output.
            print_uniq_c(output)
        return

    # --- Case 3: pattern: lines containing "frame ip:"
    # In Bash: awk '{print $3}' extracts field 3.
    if any('frame ip:' in line for line in lines):
        addresses = []
        for line in lines:
            if 'frame ip:' in line:
                parts = line.split()
                if len(parts) >= 3:
                    addr = parts[2]
                    addresses.append(addr)
        output = run_addr2line(addr2line_binary, target_file, addresses)
        if output:
            print(output, end='')
        return

    # --- Case 4: pattern: lines containing "Fault occurred at"
    # In Bash: awk '{print $6}' | cut -c3- extracts the 6th field and removes its first 2 characters.
    if any('Fault occurred at' in line for line in lines):
        addresses = []
        for line in lines:
            if 'Fault occurred at' in line:
                parts = line.split()
                if len(parts) >= 6:
                    addr_field = parts[5]
                    addr = addr_field[2:] if len(addr_field) > 2 else ""
                    if addr:
                        addresses.append(addr)
        output = run_addr2line(addr2line_binary, target_file, addresses)
        if output:
            print(output, end='')
        return

if __name__ == '__main__':
    main()

