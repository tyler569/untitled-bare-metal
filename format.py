#!/usr/bin/env python3
import os
import subprocess
import sys

# Directories to search
directories = ["arch", "include", "kern", "lib", "user"]

# File extensions to process
extensions = (".c", ".h")

def gather_files():
    files_to_format = []
    for directory in directories:
        if not os.path.isdir(directory):
            print(f"Warning: Directory '{directory}' not found, skipping.", file=sys.stderr)
            continue
        for root, _, files in os.walk(directory):
            for filename in files:
                if filename.endswith(extensions):
                    full_path = os.path.join(root, filename)
                    files_to_format.append(full_path)
    return files_to_format

def main():
    files = gather_files()
    if not files:
        print("No files found to format.")
        sys.exit(0)

    # Build the clang-format command with the -i flag and all file paths.
    cmd = ["clang-format", "-i"] + files

    try:
        subprocess.run(cmd, check=True)
        print("clang-format executed successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error: clang-format returned non-zero exit status: {e}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: clang-format not found. Is it installed and in your PATH?", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
