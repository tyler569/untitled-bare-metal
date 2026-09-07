#pragma once

#include "kern/obj/tcb.h"
#include "kern/per_cpu.h"
#include "sys/ipc.h"

// Only the current thread has a live syscall frame. Blocked threads use
// saved_state; current_user_frame can still point at an old kernel stack.
static inline frame_t *
ipc_frame (struct tcb *t)
{
  assert (t);
  if (t == this_tcb)
    {
      assert (t->current_user_frame);
      return t->current_user_frame;
    }
  return &t->saved_state;
}

static inline message_info_t
get_pending_ipc_info (struct tcb *t)
{
  // The outgoing tag remains in RSI while a sender is blocked.
  return message_info_from_word (get_frame_syscall_arg (ipc_frame (t), 1));
}

static inline void
set_ipc_result (struct tcb *t, message_info_t tag)
{
  set_frame_return (ipc_frame (t), message_info_to_word (tag));
}

static inline word_t
get_mr (word_t i)
{
  assert (this_tcb && this_tcb->ipc_buffer);

  return this_tcb->ipc_buffer->msg[i];
}

static inline word_t
get_cap (word_t i)
{
  assert (this_tcb && this_tcb->ipc_buffer);

  return this_tcb->ipc_buffer->caps_or_badges[i];
}

static inline void
set_mr (word_t i, word_t v)
{
  assert (this_tcb && this_tcb->ipc_buffer);

  this_tcb->ipc_buffer->msg[i] = v;
}

static inline void
set_ipc_info (message_info_t tag)
{
  set_ipc_result (this_tcb, tag);
}
