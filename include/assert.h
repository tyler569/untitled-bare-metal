#pragma once

#include "sys/cdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
