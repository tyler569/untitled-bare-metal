#include "kern/syscall.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/obj/endpoint.h"
#include "kern/obj/notification.h"
#include "kern/obj/tcb.h"

#include "kern/kernel_method_stubs.c"
#include "kern/syscall_dispatch.c"

#define GET_CAP(cptr, slot, err)                                              \
  err = lookup_cap_slot_this_tcb (cptr, &slot);                               \
  if (err)                                                                    \
    break;

#define ASSERT_ENDPOINT(slot)                                                 \
  if (cap_type (slot) != cap_endpoint && cap_type (slot) != cap_notification) \
    {                                                                         \
      dbg_printf ("Invalid cap type\n");                                      \
      kill_tcb (this_tcb);                                                    \
    }

void
do_syscall (uintptr_t a0, uintptr_t a1, int syscall_number)
{
  if (syscall_number != sys_debug_write && syscall_number != sys_exit)
    dbg_printf ("Task %p a0:%#lx ", this_tcb, a0);

  if (syscall_number == sys_exit)
    dbg_printf ("Task %p ", this_tcb);

  error_t err;
  cte_t *slot;

  switch (syscall_number)
    {
    case sys_exit:
      dbg_printf ("sys_exit ()\n");
      kill_tcb (this_tcb);
      schedule ();
      break;
    case sys_debug_write:
      write_debug (nullptr, (const void *)a0, a1);
      break;
    case sys_call:
      GET_CAP (a0, slot, err);
      dbg_printf ("sys_call (dest: %#lx)\n", a0);

      if (cap_type (slot) != cap_endpoint)
        dispatch_method (slot, this_tcb->ipc_buffer->tag);
      else
        invoke_endpoint_call (slot);
      break;
    case sys_send:
      GET_CAP (a0, slot, err);
      dbg_printf ("sys_send (dest: %#lx)\n", a0);

      ASSERT_ENDPOINT (slot);

      if (cap_type (slot) == cap_notification)
        invoke_notification_send (slot);
      else if (cap_type (slot) == cap_endpoint)
        invoke_endpoint_send (slot);
      break;
    case sys_nbsend:
      GET_CAP (a0, slot, err);
      dbg_printf ("sys_nbsend (dest: %#lx)\n", a0);

      ASSERT_ENDPOINT (slot);

      if (cap_type (slot) == cap_endpoint)
        invoke_endpoint_nbsend (slot);
      break;
    case sys_recv:
      GET_CAP (a0, slot, err);
      dbg_printf ("sys_recv (dest: %#lx)\n", a0);

      ASSERT_ENDPOINT (slot);

      if (cap_type (slot) == cap_notification)
        invoke_notification_recv (slot);
      else if (cap_type (slot) == cap_endpoint)
        invoke_endpoint_recv (slot);
      break;
    case sys_nbrecv:
      GET_CAP (a0, slot, err);
      dbg_printf ("sys_nbrecv (dest: %#lx)\n", a0);

      ASSERT_ENDPOINT (slot);

      if (cap_type (slot) == cap_endpoint)
        invoke_endpoint_nbrecv (slot);
      break;
    case sys_reply:
      dbg_printf ("sys_reply (info: %#lx)\n", a0);
      invoke_reply ();
      break;
    case sys_replyrecv:
      GET_CAP (a0, slot, err);
      dbg_printf ("sys_replyrecv (dest: %#lx)\n", a0);

      ASSERT_ENDPOINT (slot);

      invoke_reply_recv (slot);
      break;
    case sys_yield:
      dbg_printf ("sys_yield ()\n");
      schedule ();
      break;
    default:
      err_printf ("Invalid syscall number: %d\n", syscall_number);
      kill_tcb (this_tcb);
    }
}
