#!/usr/bin/env bash

file="${1-build/kern/untitled_bare_metal}"
objdump_binary="llvm-objdump"
format="intel"

if [ ! -f "$file" ]; then
    echo "Error: $file does not exist"
    echo "Usage: $0 [file]"
    exit 1
fi

intel_option=""
if [ "$format" == "intel" ]; then
    intel_option="-Mintel"
fi

"$objdump_binary" -d -S "$intel_option" -j.text -j.text.low -j.init -j.fini "$file" | less
