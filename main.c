#include "kernel.h"
#include <assert.h>

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

  init_kmem_alloc ();

  int *ptr = kmem_alloc (sizeof (int));
  *ptr = 42;
  printf ("kmem_alloc: %p: %i\n", ptr, *ptr);
  kmem_free (ptr);

  assert ("foo bar");
  assert (0 && "foo bar");

  asm volatile ("int $0x3");

  halt_forever ();
}
