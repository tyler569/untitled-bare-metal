#include "kernel.h"

void
panic (const char *msg, ...)
{
  va_list args;
  va_start (args, msg);

  printf ("PANIC: ");
  vprintf (msg, args);
  va_end (args);

  halt_forever ();
}