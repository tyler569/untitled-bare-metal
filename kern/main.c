#include "kern/kernel.h"
#include "kern/obj/tcb.h"
#include "rng.h"
#include "stdio.h"

void
kernel_main ()
{
  printf ("Hello, World!\n");

  run_smoke_tests ();

  void *initrd;
  size_t initrd_size;
  if (get_initrd_info (&initrd, &initrd_size))
    {
      printf ("Initrd found at %p, size %zu\n", initrd, initrd_size);
      create_init_tcb (initrd);
      schedule ();
    }
  else
    printf ("No initrd found\n");

  printf ("Idle loop\n");
  halt_forever ();
}
