#include "kern/kernel.h"
#include "kern/task.h"
#include "rng.h"
#include "stdio.h"

struct task init_task;

void
kernel_main ()
{
  printf ("Hello, World!\n");

  init_tasks ();

  run_smoke_tests ();

  void *initrd;
  size_t initrd_size;
  if (get_initrd_info (&initrd, &initrd_size))
    {
      printf ("Initrd found at %p, size %zu\n", initrd, initrd_size);
      create_task_from_elf_in_this_vm (&init_task, initrd);
      make_task_runnable (&init_task);
      schedule ();
    }
  else
    printf ("No initrd found\n");

  printf ("Idle loop\n");
  halt_forever ();
}
