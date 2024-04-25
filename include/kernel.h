#pragma once

#include "stdio.h"
#include <stdint.h>

void kernel_main ();

uintptr_t alloc_page ();
void free_page (uintptr_t);

[[noreturn]] void panic (const char *msg, ...);

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_ (x)
#define FILE_AND_LINE FILE_BASENAME ":" STRINGIFY (__LINE__)

#define assert(x)                                                             \
  do                                                                          \
    {                                                                         \
      if (!(x))                                                               \
        panic (FILE_AND_LINE ": assertion failed: %s", #x);                   \
    }                                                                         \
  while (0)

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))

// arch-specific procedures provided by arch code.

[[noreturn]] void halt_forever ();

void relax_busy_loop ();

ssize_t write_debug (FILE *, const void *str, size_t len);
