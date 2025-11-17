#!/usr/bin/env bash

find include kern lib user -name *.[ch] | xargs clang-format -i
