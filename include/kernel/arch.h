#pragma once

#include <stdio.h>

[[noreturn]] void halt_forever ();

ssize_t arch_debug_write (FILE *, const void *str, size_t len);