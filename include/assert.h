#pragma once

#include <sys/cdefs.h>

[[noreturn]] void panic (const char *msg, ...);

#define assert(x)                                                             \
  do                                                                          \
    {                                                                         \
      if (!(x))                                                               \
        panic (FILE_AND_LINE ": assertion failed: %s", #x);                   \
    }                                                                         \
  while (0)
