#!/usr/bin/env bash

find arch include kern lib user -name *.[ch] | xargs clang-format -i
