#include "assert.h"
#include "elf.h"
#include "kernel.h"
#include "rng.h"
#include "stdio.h"
#include "sys/arch.h"
#include "sys/mem.h"

void
kernel_main ()
{
  printf ("Hello, World!\n");

  init_random (1);

  run_smoke_tests ();

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

  printf ("Idle loop\n");
  halt_forever ();
}
