#include "stdio.h"
#include "sys/arch.h"
#include "sys/cdefs.h"
#include "sys/task.h"

uintptr_t
do_syscall (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
            uintptr_t a4, uintptr_t a5, int syscall_number, frame_t *)
{
  (void)a2, (void)a3, (void)a4, (void)a5;

  switch (syscall_number)
    {
    case 0:
      printf ("Exit (num: %i)\n", syscall_number);
      kill_task (this_cpu->current_task);
      schedule ();
      break;
    case 1:
      printf ("Write (num: %i, str: %#lx, len: %lu)\n", syscall_number, a0,
              a1);
      printf ("  -> \"%.*s\"\n", (int)a1, (const char *)a0);
      break;
    case 2:
      printf ("Clone (num: %i, fn: %#lx, stk: %#lx)\n", syscall_number, a0,
              a1);
      struct task *new_t = create_task_in_this_vm (a0, a1);
      make_task_runnable (new_t);
      schedule ();
      break;
    case 3:
      printf ("Yield (num: %i)\n", syscall_number);
      schedule ();
      break;
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);
      printf ("Unknown syscall\n");
      break;
    }

  return 0;
}
