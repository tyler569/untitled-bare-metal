#include "kernel.h"
#include "rng.h"
#include "stdio.h"
#include "sys/task.h"

void
kernel_main ()
{
  printf ("Hello, World!\n");

  init_random (1);
  init_tasks ();

  run_smoke_tests ();

  void *initrd;
  size_t initrd_size;
  if (get_initrd_info (&initrd, &initrd_size))
    {
      printf ("Initrd found at %p, size %zu\n", initrd, initrd_size);
      struct task *t = create_task_from_elf_in_this_vm (initrd);
      make_task_runnable (t);
      schedule ();
    }
  else
    printf ("No initrd found\n");

  printf ("Idle loop\n");
  halt_forever ();
}
