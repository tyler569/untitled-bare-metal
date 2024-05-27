#!/usr/bin/env bash

find arch include kernel lib user -name *.[ch] | xargs clang-format -i
