#include "kern/syscall.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/per_cpu.h"
#include "stdio.h"

int invoke_untyped_cap (cap_t cap, word_t label);
int invoke_tcb_method (cap_t cap, word_t label);

void
return_ipc_error (word_t err, word_t registers)
{
  set_ipc_info (new_message_info (err, 0, registers));
}

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
      printf ("Exit (num: %i)\n", syscall_number);
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

        printf ("Call (num: %i, cptr: %#lx, info: %#lx) ", syscall_number,
                cptr, info);

        cap_t cap;
        exception_t status
            = lookup_cap (this_tcb->cspace_root, cptr, 64, &cap);
        if (status != no_error)
          {
            printf ("-> Error\n");
            return_ipc_error (status, 0);
            return 1;
          }

        switch (cap.type)
          {
          case cap_untyped:
            printf ("-> Untyped\n");
            invoke_untyped_cap (cap, get_message_label (info));
            break;
          case cap_tcb:
            printf ("-> TCB\n");
            invoke_tcb_method (cap, get_message_label (info));
            break;
          default:
            printf ("-> Unknown\n");
            return_ipc_error (invalid_capability, 0);
            return 1;
          }
        return 0;
      }
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);

      printf ("Unknown syscall\n");
      break;
    }

  return ret;
}
