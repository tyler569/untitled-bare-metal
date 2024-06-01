#include "kern/syscall.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/per_cpu.h"
#include "stdio.h"

#include "kern/kernel_method_stubs.c"
#include "kern/methods.h"
#include "kern/syscall_dispatch.c"

uintptr_t
do_syscall (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
            uintptr_t a4, uintptr_t a5, int syscall_number, frame_t *f)
{
  (void)a2, (void)a3, (void)a4, (void)a5, (void)f;

  uintptr_t ret = 0;

  if (syscall_number != sys_debug_write)
    printf ("Task %#lx ", (uintptr_t)this_tcb & 0xff);

  switch (syscall_number)
    {
    case sys_exit:
      printf ("Exit ()\n");
      kill_tcb (this_tcb);
      schedule ();
      UNREACHABLE ();
    case sys_debug_write:
      // printf ("Write (num: %i, str: %#lx, len: %lu)\n", syscall_number, a0,
      //         a1);
      // printf ("  -> \"%.*s\"\n", (int)a1, (const char *)a0);
      write_debug (nullptr, (const void *)a0, a1);
      break;
    case sys_call:
      {
        cptr_t cptr = (cptr_t)a0;
        message_info_t info = (message_info_t)a1;

        printf ("Call (dest: %#lx, info: %#lx)\n", cptr, info);

        dispatch_method (cptr, info);
        return 0;
      }
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);

      printf ("Unknown syscall\n");
      break;
    }

  return ret;
}
