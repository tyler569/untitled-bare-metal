#include "sys/syscall.h"
#include "stdio.h"
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
      UNREACHABLE ();
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
    case 4:
      printf ("Send (num: %i, to: %lu, msg: %#lx)\n", syscall_number, a0, a1);
      // send_message (a0, a1);
      break;
    case 5:
      printf ("Receive (num: %i, from: %lu, msg: %#lx)\n", syscall_number, a0,
              a1);
      // receive_message (a0, a1);
      break;
    case 6:
      printf ("Create (num: %i, vm: %lu)\n", syscall_number, a0);
      // struct task *t = create_task_from_syscall (a0 != 0);
      break;
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);

      printf ("Unknown syscall\n");
      break;
    }

  return 0;
}
