#pragma once

#include "sys/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/per_cpu.h"

static inline message_info_t
get_ipc_info ()
{
  assert (this_tcb && this_tcb->ipc_buffer);

  return this_tcb->ipc_buffer->tag;
}

static inline word_t
get_ipc_label ()
{
  return get_message_label (get_ipc_info ());
}

static inline word_t
get_ipc_extra_caps ()
{
  return get_message_extra_caps (get_ipc_info ());
}

static inline word_t
get_ipc_length ()
{
  return get_message_length (get_ipc_info ());
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
  assert (this_tcb && this_tcb->ipc_buffer);

  this_tcb->ipc_buffer->tag = tag;
}
