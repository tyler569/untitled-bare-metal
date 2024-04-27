#include "kernel.h"
#include "stdarg.h"
#include "stdio.h"

[[noreturn]] void
panic (const char *msg, ...)
{
  va_list args;
  va_start (args, msg);

  printf ("PANIC: ");
  vprintf (msg, args);
  va_end (args);

  printf ("\n");

  halt_forever ();
}
