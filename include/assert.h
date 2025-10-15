#pragma once

#include "sys/cdefs.h"

[[noreturn]] void panic (const char *msg, ...);

#define assert(x)                                                             \
  do                                                                          \
    {                                                                         \
      if (!(x))                                                               \
        panic (__FILE__                                                       \
               ":" STRINGIFY (__LINE__) ": %s: assertion failed: %s",         \
               __func__, #x);                                                 \
    }                                                                         \
  while (0)

#define assert_eq(x, y) assert ((x) == (y))
