#include "kernel.h"

void
kernel_main ()
{
  printf ("Hello, World!\n");

  uintptr_t page;
  printf ("%lu\n", page = alloc_page ());
  free_page (page);
  printf ("%lu\n", page = alloc_page ());
  free_page (page);

  asm volatile ("int $0x3");

  halt_forever ();
}