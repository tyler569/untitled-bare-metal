#pragma once

#include "stdio.h"
#include "sys/cdefs.h"

[[noreturn]] void panic (const char *msg, ...);

#define assert(x)                                                             \
  do                                                                          \
    {                                                                         \
      if (!(x))                                                               \
        {                                                                     \
          printf (FILE_AND_LINE ": assertion failed: %s\n", #x);              \
          asm volatile ("int $255");                                          \
          panic ("assertion failed");                                         \
        }                                                                     \
    }                                                                         \
  while (0)

static inline void
assert_eq (long x, long y)
{
  if (x != y)
    panic (FILE_AND_LINE ": assertion failed: %ld != %ld", x, y);
}
