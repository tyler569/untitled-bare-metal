#include "sys/syscall.h"
#include "stdio.h"
#include "sys/per_cpu.h"
#include "sys/task.h"

uintptr_t
do_syscall (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
            uintptr_t a4, uintptr_t a5, int syscall_number, frame_t *f)
{
  (void)a2, (void)a3, (void)a4, (void)a5, (void)f;

  uintptr_t ret = 0;

  if (syscall_number != 1)
    printf ("Task %#lx ", (uintptr_t)this_task & 0xff);

  switch (syscall_number)
    {
    case 0:
      printf ("Exit (num: %i)\n", syscall_number);
      kill_task (this_task);
      schedule ();
      UNREACHABLE ();
    case 1:
      // printf ("Write (num: %i, str: %#lx, len: %lu)\n", syscall_number, a0,
      //         a1);
      // printf ("  -> \"%.*s\"\n", (int)a1, (const char *)a0);
      write_debug (nullptr, (const void *)a0, a1);
      break;
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);

      printf ("Unknown syscall\n");
      break;
    }

  return ret;
}
