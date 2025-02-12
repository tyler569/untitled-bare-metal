#include "stdio.h"

#include "./lib.h"

struct ipc_buffer *__ipc_buffer;

enum
{
  calculator_quit = 0,
  calculator_ret42 = 1,
  calculator_double = 2,
  calculator_inc = 3,
  calculator_add = 4,

  calculator_error_unknown = 5,
};

