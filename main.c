#include "kernel.h"

void
kernel_main ()
{
  printf ("Hello, World!\n");

  uintptr_t pages[10];

  for (int i = 0; i < 5; i++)
    printf ("page %i: %lx\n", i, pages[i] = alloc_page ());

  for (int i = 0; i < 5; i++)
    free_page (pages[i]);

  for (int i = 0; i < 10; i++)
    printf ("page %i: %lx\n", i, pages[i] = alloc_page ());

  for (int i = 0; i < 10; i++)
    free_page (pages[i]);

  assert ("foo bar");
  assert (0 && "foo bar");

  asm volatile ("int $0x3");

  halt_forever ();
}
