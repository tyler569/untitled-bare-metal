#include "kern/obj/endpoint.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"
#include "string.h"

static inline struct tcb *
first_tcb (struct endpoint *e)
{
  return CONTAINER_OF (e->list.next, struct tcb, send_receive_node);
}

static void
send_message_directly (struct tcb *receiver, word_t info, word_t badge,
                       bool resume_now)
{
  assert (receiver->ipc_buffer && "Receiver has no IPC buffer");
  assert (this_tcb->ipc_buffer && "Sender has no IPC buffer");

  word_t size = get_message_length (info) * sizeof (word_t);
  memcpy (receiver->ipc_buffer->msg, this_tcb->ipc_buffer->msg, size);

  receiver->ipc_buffer->tag = info;
  receiver->saved_state.rax = info;
  receiver->saved_state.rdi = badge;

  if (this_tcb->expects_reply)
    receiver->reply_to = this_tcb;

  if (resume_now)
    {
      receiver->state = TASK_STATE_RUNNABLE;
      switch_tcb (receiver);
    }
  else
    make_tcb_runnable (receiver);
}

static void
send_message_to_blocked_receiver (struct endpoint *e, word_t info,
                                  word_t badge, bool resume_now)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *receiver = CONTAINER_OF (next, struct tcb, send_receive_node);

  send_message_directly (receiver, info, badge, resume_now);
}

static void
queue_message_on_endpoint (struct endpoint *e, word_t info, word_t badge,
                           bool is_call)
{
  append_to_list (&this_tcb->send_receive_node, &e->list);

  if (is_call)
    this_tcb->state = TASK_STATE_CALLING;
  else
    this_tcb->state = TASK_STATE_SENDING;

  if (this_tcb->ipc_buffer)
    this_tcb->ipc_buffer->tag = info;

  this_tcb->endpoint_badge = badge;

  schedule ();
}

static message_info_t
receive_message_from_blocked_sender (struct endpoint *e, word_t *badge)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *sender = CONTAINER_OF (next, struct tcb, send_receive_node);

  message_info_t info = (message_info_t)sender->ipc_buffer->tag;

  word_t size = get_message_length (info) * sizeof (word_t);
  memcpy (this_tcb->ipc_buffer->msg, sender->ipc_buffer->msg, size);

  this_tcb->ipc_buffer->tag = info;
  this_tcb->current_user_frame->rax = info;
  this_tcb->current_user_frame->rdi = sender->endpoint_badge;
  *badge = sender->endpoint_badge;

  if (sender->expects_reply)
    this_tcb->reply_to = sender;
  else
    make_tcb_runnable (sender);

  return info;
}

static void
queue_receiver_on_endpoint (struct endpoint *e)
{
  append_to_list (&this_tcb->send_receive_node, &e->list);
  this_tcb->state = TASK_STATE_RECEIVING;
  schedule ();
}

static inline bool
is_receive_blocked (struct endpoint *e)
{
  return is_list_empty (&e->list)
         || first_tcb (e)->state == TASK_STATE_RECEIVING;
}

static inline bool
is_send_blocked (struct endpoint *e)
{
  return is_list_empty (&e->list)
         || first_tcb (e)->state == TASK_STATE_SENDING;
}

static void
endpoint_send (struct endpoint *e, word_t message_info, word_t badge,
               bool is_call)
{
  if (is_send_blocked (e))
    queue_message_on_endpoint (e, message_info, badge, is_call);
  else
    send_message_to_blocked_receiver (e, message_info, badge, true);
}

static message_info_t
endpoint_recv (struct endpoint *e, word_t *sender)
{
  if (is_receive_blocked (e))
    queue_receiver_on_endpoint (e);
  else
    return receive_message_from_blocked_sender (e, sender);
  return 0;
}

static void
maybe_init_endpoint (struct endpoint *e)
{
  if (e->list.next)
    return;

  init_list (&e->list);
}

void
invoke_endpoint_send (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, false);
}

message_info_t
invoke_endpoint_recv (cte_t *cap, word_t *sender)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  return endpoint_recv (e, sender);
}

message_info_t
invoke_endpoint_call (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = true;
  this_tcb->state = TASK_STATE_CALLING;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, true);
  return 0;
}

void
invoke_reply (word_t message_info)
{
  // TODO: this should use tcb->reply capability

  struct tcb *receiver = this_tcb->reply_to;
  if (!receiver || receiver->state != TASK_STATE_CALLING)
    {
      printf ("Failed to deliver reply!\n");
      kill_tcb (this_tcb);
    }
  send_message_directly (receiver, message_info, 0, false);
}

message_info_t
invoke_reply_recv (cte_t *cap, word_t message_info, word_t *sender)
{
  invoke_reply (message_info);
  return invoke_endpoint_recv (cap, sender);
}
