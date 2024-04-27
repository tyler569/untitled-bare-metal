#!/usr/bin/env bash

set -euo pipefail

bin=${1-cmake-build-debug/untitled_bare_metal}

grep 'frame ip:' last_output | awk '{print $3}' \
  | xargs llvm-addr2line -fips -e $bin
