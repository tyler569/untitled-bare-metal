#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys

def main():
    parser = argparse.ArgumentParser(
        description="Disassemble a binary using llvm-objdump and view output via less.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "file",
        nargs="?",
        default="build/kern/untitled_bare_metal",
        help="Target file for llvm-objdump"
    )
    parser.add_argument(
        "--format",
        default="att",
        choices=["att", "intel"],
        help="Assembly format"
    )
    parser.add_argument(
        "-f", "--functions",
        nargs="+",
        type=str,
        help="Filter functions to disassemble"
    )
    args = parser.parse_args()

    target_file = args.file
    assembly_format = args.format

    if not os.path.isfile(target_file):
        print(f"Error: {target_file} does not exist", file=sys.stderr)
        print(f"Usage: {sys.argv[0]} [file]", file=sys.stderr)
        sys.exit(1)

    objdump_binary = "llvm-objdump"

    # Set intel_option if the format is intel
    intel_option = ""
    if assembly_format == "intel":
        intel_option = "-Mintel"

    # Build the command
    cmd = [objdump_binary, "-d", "-S"]
    if intel_option:
        cmd.append(intel_option)
    cmd.extend(["-j.text", "-j.text.low", "-j.init", "-j.fini", target_file])

    # Filter functions
    if args.functions:
        cmd.append("--disassemble-symbols=" + ",".join(args.functions))

    # Execute the command and pipe the output to less
    try:
        # Launch llvm-objdump; its stdout is piped to less.
        p1 = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        p2 = subprocess.Popen(["less"], stdin=p1.stdout)
        # Close p1.stdout in the parent so that p1 receives a SIGPIPE if less exits.
        p1.stdout.close()
        p2.communicate()
    except KeyboardInterrupt:
        sys.exit(0)
    except Exception as e:
        print(f"Error executing command: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
