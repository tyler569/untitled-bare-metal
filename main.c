#include "kernel.h"
#include <assert.h>

void
kernel_main ()
{
  printf ("Hello, World!\n");

  init_kmem_alloc ();

  int *allocations[512];
  for (int i = 0; i < 512; i++)
    {
      allocations[i] = kmem_alloc (sizeof (int));
      assert (allocations[i]);

      *allocations[i] = i;
    }

  for (int i = 0; i < 512; i++)
    {
      assert (*allocations[i] == i);
      kmem_free (allocations[i]);
    }

  void *biig_buffer = kmem_alloc (1024 * 1024);
  assert (biig_buffer);
  for (int i = 0; i < 1024 * 1024; i++)
    ((char *)biig_buffer)[i] = i % 256;

  kmem_free (biig_buffer);

  assert ("foo bar");

  asm volatile ("int $0x3");

  printf ("Idle loop\n");

  halt_forever ();
}
