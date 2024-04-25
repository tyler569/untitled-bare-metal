#pragma once

#include "stdio.h"
#include <stdint.h>

void kernel_main ();

uintptr_t alloc_page ();
void free_page (uintptr_t);

[[noreturn]] void panic (const char *msg, ...);

// arch-specific procedures provided by arch code.

[[noreturn]] void halt_forever ();

void cpu_relax ();

ssize_t write_debug (FILE *, const void *str, size_t len);