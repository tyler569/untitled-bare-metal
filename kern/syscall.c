#include "kern/syscall.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/methods.h"
#include "kern/obj/endpoint.h"
#include "kern/obj/notification.h"
#include "kern/obj/tcb.h"

#include "kern/syscall_dispatch.c"

static inline void
assert_endpoint (cte_t *slot)
{
  if (cap_type (slot) != cap_endpoint && cap_type (slot) != cap_notification)
    {
      dbg_printf ("Invalid cap type\n");
      kill_tcb (this_tcb);
      schedule ();
    }
}

static message_info_t
op_syscall (uintptr_t a0, uintptr_t a1, enum syscall_number syscall_number,
            bool *should_return)
{
  if (syscall_number != sys_debug_write && syscall_number != sys_exit)
    dbg_printf ("Task %p a0:%#lx ", this_tcb, a0);

  if (syscall_number == sys_exit)
    dbg_printf ("Task %p ", this_tcb);

  // the syscalls without a capability handle in a0
  switch (syscall_number)
    {
    case sys_exit:
      dbg_printf ("sys_exit ()\n");

      kill_tcb (this_tcb);
      schedule ();

      return msg_noreturn ();
    case sys_debug_write:
      write_debug (nullptr, (const void *)a0, a1);

      return msg_ok (0);
    case sys_yield:
      dbg_printf ("sys_yield ()\n");

      schedule ();

      return msg_noreturn ();
    case sys_reply:
      dbg_printf ("sys_reply (info: %#lx)\n", a0);

      *should_return = false;
      invoke_reply ();

      return msg_ok (0);
    default:
    }

  cte_t *slot;
  TRY (lookup_cap_slot_this_tcb (a0, &slot));

  switch (syscall_number)
    {
    case sys_call:
      {
        dbg_printf ("sys_call (dest: %#lx)\n", a0);

        if (cap_type (slot) != cap_endpoint)
          return dispatch_method (slot, this_tcb->ipc_buffer->tag);
        else
          {
            *should_return = false;
            invoke_endpoint_call (slot);
          }
        break;
      }
    case sys_send:
      {
        dbg_printf ("sys_send (dest: %#lx)\n", a0);
        *should_return = false;

        assert_endpoint (slot);

        if (cap_type (slot) == cap_notification)
          invoke_notification_send (slot);
        else if (cap_type (slot) == cap_endpoint)
          invoke_endpoint_send (slot);
        break;
      }
    case sys_nbsend:
      {
        dbg_printf ("sys_nbsend (dest: %#lx)\n", a0);
        *should_return = false;

        assert_endpoint (slot);

        if (cap_type (slot) == cap_endpoint)
          invoke_endpoint_nbsend (slot);
        break;
      }
    case sys_recv:
      {
        dbg_printf ("sys_recv (dest: %#lx)\n", a0);

        assert_endpoint (slot);

        if (cap_type (slot) == cap_notification)
          return invoke_notification_recv (slot);
        else if (cap_type (slot) == cap_endpoint)
          return invoke_endpoint_recv (slot);
        break;
      }
    case sys_nbrecv:
      {
        dbg_printf ("sys_nbrecv (dest: %#lx)\n", a0);

        assert_endpoint (slot);

        if (cap_type (slot) == cap_endpoint)
          return invoke_endpoint_nbrecv (slot);
        break;
      }
    case sys_replyrecv:
      {
        dbg_printf ("sys_replyrecv (dest: %#lx)\n", a0);

        assert_endpoint (slot);

        return invoke_reply_recv (slot);
      }
    default:
      err_printf ("Invalid syscall number: %d\n", syscall_number);
      kill_tcb (this_tcb);
    }

  return msg_ok (0);
}

void
do_syscall (uintptr_t a0, uintptr_t a1, enum syscall_number syscall_number)
{
  bool should_return = true;
  message_info_t tag = op_syscall (a0, a1, syscall_number, &should_return);
  if (should_return)
    this_tcb->ipc_buffer->tag = tag;
}
