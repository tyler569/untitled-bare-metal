#include "kernel/main.h"
#include "kernel/arch.h"
#include <sys/cdefs.h>

[[noreturn]] USED void
kernel_main ()
{
  printf ("Hello, world!\n");

  asm volatile ("int3");

  halt_forever ();
}
