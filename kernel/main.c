#include "assert.h"
#include "elf.h"
#include "kernel.h"
#include "stdio.h"
#include "sys/arch.h"
#include "sys/mem.h"

void
kernel_main ()
{
  printf ("Hello, World!\n");

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

  void *initrd;
  size_t initrd_size;
  if (get_initrd_info (&initrd, &initrd_size))
    {
      printf ("Initrd found at %p, size %zu\n", initrd, initrd_size);
      elf_load (initrd);

      uintptr_t stack = 0x7ffffff00000;

      uintptr_t root = get_vm_root ();
      add_vm_mapping (root, stack, alloc_page (),
                      PTE_PRESENT | PTE_USER | PTE_WRITE);

      uintptr_t entry = elf_entry (initrd);
      printf ("Entry point: %#lx\n", entry);
      jump_to_userland (entry, stack + PAGE_SIZE);
    }
  else
    printf ("No initrd found\n");

  debug_trap ();

  printf ("Idle loop\n");

  halt_forever ();
}
