#include "kern/kernel.h"
#include "kern/obj/tcb.h"
#include "rng.h"
#include "stdio.h"

struct tcb init_tcb;

void
kernel_main ()
{
  printf ("Hello, World!\n");

  init_tcbs ();

  run_smoke_tests ();

  void *initrd;
  size_t initrd_size;
  if (get_initrd_info (&initrd, &initrd_size))
    {
      printf ("Initrd found at %p, size %zu\n", initrd, initrd_size);
      create_tcb_from_elf_in_this_vm (&init_tcb, initrd);
      make_tcb_runnable (&init_tcb);
      schedule ();
    }
  else
    printf ("No initrd found\n");

  printf ("Idle loop\n");
  halt_forever ();
}
