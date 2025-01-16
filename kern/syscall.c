#include "kern/syscall.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/endpoint.h"
#include "kern/obj/tcb.h"
#include "kern/per_cpu.h"
#include "stdio.h"

#include "kern/kernel_method_stubs.c"
#include "kern/methods.h"
#include "kern/syscall_dispatch.c"

#define GET_CAP(cptr, slot, err)                                              \
  slot = lookup_cap_slot_this_tcb (cptr, &err);                               \
  if (err != no_error)                                                        \
    {                                                                         \
      return_ipc (err, 0);                                                    \
      return err;                                                             \
    }

uintptr_t
do_syscall (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
            uintptr_t a4, uintptr_t a5, int syscall_number, frame_t *f)
{
  (void)a2, (void)a3, (void)a4, (void)a5, (void)f;

  uintptr_t ret = 0;

  if (this_tcb)
    this_tcb->current_user_frame = f;

  if (syscall_number != sys_debug_write && syscall_number != sys_exit)
    printf ("Task %#lx a0:%#lx ", (uintptr_t)this_tcb & 0xfff, a0);

  if (syscall_number == sys_exit)
    printf ("Task %#lx ", (uintptr_t)this_tcb & 0xfff);

  error_t err;
  cte_t *slot;

  switch (syscall_number)
    {
    case sys_exit:
      printf ("sys_exit ()\n");
      kill_tcb (this_tcb);
      schedule ();
      unreachable ();
    case sys_debug_write:
      write_debug (nullptr, (const void *)a0, a1);
      return no_error;
    case sys_call:
      GET_CAP (a0, slot, err);

      if (cap_type (slot) != cap_endpoint)
        return dispatch_method (slot, a1);
      else
        {
          printf ("sys_call (dest: %#lx, info: %#lx)\n", a0, a1);
          invoke_endpoint_call (slot, a1);
        }
    case sys_reply:
      printf ("sys_reply (info: %#lx)\n", a0);
      return invoke_reply (nullptr, a0); // TODO TCB reply cap
    case sys_send:
      GET_CAP (a0, slot, err);
      printf ("sys_send (dest: %#lx, info: %#lx)\n", a0, a1);

      if (cap_type (slot) != cap_endpoint)
        {
          return_ipc (illegal_operation, 0);
          return illegal_operation;
        }

      return invoke_endpoint_send (slot, a1);
    case sys_recv:
      GET_CAP (a0, slot, err);

      printf ("sys_recv (dest: %#lx)\n", a0);

      if (cap_type (slot) != cap_endpoint)
        {
          return_ipc (illegal_operation, 0);
          return illegal_operation;
        }

      return invoke_endpoint_recv (slot);
    case sys_yield:
      printf ("sys_yield ()\n");
      schedule ();
      unreachable ();
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);
      return no_error;
    }

  return ret;
}
