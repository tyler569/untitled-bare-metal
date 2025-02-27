#pragma once

#include "sys/cdefs.h"

[[noreturn]] void panic (const char *msg, ...);

#define assert(x)                                                             \
  do                                                                          \
    {                                                                         \
      if (!(x))                                                               \
        panic (FILE_AND_LINE ": assertion failed: %s", #x);                   \
    }                                                                         \
  while (0)

static inline void
assert_eq (long x, long y)
{
  if (x != y)
    panic (FILE_AND_LINE ": assertion failed: %ld != %ld", x, y);
}
