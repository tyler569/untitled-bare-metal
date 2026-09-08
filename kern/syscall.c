#include "kern/syscall.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/methods.h"
#include "kern/obj/endpoint.h"
#include "kern/obj/notification.h"
#include "kern/obj/tcb.h"

#include "kern/syscall_dispatch.c"

static message_info_t
op_syscall (const uintptr_t a0, const uintptr_t a1,
            const enum syscall_number syscall_number)
{
  if (syscall_number != SYS_DEBUG_WRITE && syscall_number != SYS_EXIT)
    dbg_printf ("Task %p a0:0x%lX ", this_tcb, a0);

  if (syscall_number == SYS_EXIT)
    dbg_printf ("Task %p ", this_tcb);

  const message_info_t info = message_info_from_word (a1);

  // the syscalls without a capability handle in a0
  switch (syscall_number)
    {
    case SYS_EXIT:
      dbg_printf ("sys_exit ()\n");

      kill_tcb (this_tcb);
      schedule ();

      return msg_noreturn ();
    case SYS_DEBUG_WRITE:
      write_debug (nullptr, (const void *)a0, a1);

      return msg_ok (0);
    case SYS_YIELD:
      dbg_printf ("sys_yield ()\n");

      schedule ();

      return msg_noreturn ();
    case SYS_REPLY:
      dbg_printf ("sys_reply (info: 0x%lX)\n", message_info_to_word (info));

      invoke_reply (info);

      return info;
    default:
    }

  cte_t *slot;
  TRY (lookup_cap_slot_this_tcb (a0, &slot));

  switch (syscall_number)
    {
    case SYS_CALL:
      {
        dbg_printf ("sys_call (dest: 0x%lX)\n", a0);

        if (cap_type (slot) != CAP_ENDPOINT)
          return dispatch_method (slot, info);

        invoke_endpoint_call (slot, info);
        return msg_noreturn ();
      }
    case SYS_SEND:
      {
        dbg_printf ("sys_send (dest: 0x%lX)\n", a0);

        if (cap_type (slot) != CAP_NOTIFICATION
            && cap_type (slot) != CAP_ENDPOINT)
          return msg_err (INVALID_CAPABILITY, 0);

        if (cap_type (slot) == CAP_NOTIFICATION)
          invoke_notification_send (slot);
        else if (cap_type (slot) == CAP_ENDPOINT)
          invoke_endpoint_send (slot, info);

        return msg_noreturn ();
      }
    case SYS_NBSEND:
      {
        dbg_printf ("sys_nbsend (dest: 0x%lX)\n", a0);

        if (cap_type (slot) != CAP_ENDPOINT)
          return msg_err (INVALID_CAPABILITY, 0);

        invoke_endpoint_nbsend (slot, info);
        return msg_noreturn ();
      }
    case SYS_RECV:
      {
        dbg_printf ("sys_recv (dest: 0x%lX)\n", a0);

        if (cap_type (slot) != CAP_NOTIFICATION
            && cap_type (slot) != CAP_ENDPOINT)
          return msg_err (INVALID_CAPABILITY, 0);

        if (cap_type (slot) == CAP_NOTIFICATION)
          return invoke_notification_recv (slot);
        else if (cap_type (slot) == CAP_ENDPOINT)
          return invoke_endpoint_recv (slot);
      }
    case SYS_NBRECV:
      {
        dbg_printf ("sys_nbrecv (dest: 0x%lX)\n", a0);

        if (cap_type (slot) != CAP_ENDPOINT)
          return msg_err (INVALID_CAPABILITY, 0);

        return invoke_endpoint_nbrecv (slot);
      }
    case SYS_REPLYRECV:
      {
        dbg_printf ("sys_replyrecv (dest: 0x%lX)\n", a0);

        if (cap_type (slot) != CAP_ENDPOINT)
          return msg_err (INVALID_CAPABILITY, 0);

        return invoke_reply_recv (slot, info);
      }
    default:
      err_printf ("Invalid syscall number: %d\n", syscall_number);
      return msg_err (INVALID_SYSCALL, 0);
    }
}

void
do_syscall (uintptr_t a0, uintptr_t a1, enum syscall_number syscall_number)
{
  const message_info_t tag = op_syscall (a0, a1, syscall_number);
  if (!msg_is_noreturn (tag))
    set_ipc_result (this_tcb, tag);
}
