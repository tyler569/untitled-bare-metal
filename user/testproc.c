#include "stdio.h"

#include "./lib.h"

extern inline long write (FILE *, const void *str, unsigned long len);

struct ipc_buffer *__ipc_buffer;

int
main ()
{
  printf ("Hello from testproc\n");
  return 0;
}

void
c_start (void *ipc_buffer)
{
  __ipc_buffer = ipc_buffer;

  main ();
  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ( // "movq %%rdi, %%rsp\n\t"
      "nop\n\tnop\n\t"
      "jmp c_start" ::
          : "rsp");
}
