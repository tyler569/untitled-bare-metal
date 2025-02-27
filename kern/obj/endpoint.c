#include "kern/obj/endpoint.h"
#include "kern/cap.h"
#include "kern/obj/notification.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"
#include "string.h"

static inline struct tcb *
first_tcb (struct endpoint *e)
{
  return CONTAINER_OF (e->list.next, struct tcb, send_receive_node);
}

static void
transfer_message (message_info_t info, struct tcb *sender,
                  struct tcb *receiver, frame_t *receiver_frame, word_t badge)
{
  word_t size = get_message_length (info) * sizeof (word_t);
  memcpy (receiver->ipc_buffer->msg, sender->ipc_buffer->msg, size);

  receiver->ipc_buffer->tag = info;
  receiver_frame->rax = info;
  receiver_frame->rdi = badge;

  word_t transfer_cap = get_message_extra_caps (sender->ipc_buffer->tag);
  if (transfer_cap)
    {
      error_t err;
      cptr_t send_cptr = sender->ipc_buffer->caps_or_badges[0];
      cte_t *cap = lookup_cap_slot (&sender->cspace_root, send_cptr, 64, &err);

      if (err)
        {
          printf ("Failed to lookup cap for transfer\n");
          return;
        }

      cte_t *recv_cnode_root
          = lookup_cap_slot (&receiver->cspace_root,
                             receiver->ipc_buffer->receive_cnode, 64, &err);
      if (err)
        {
          printf ("Failed to lookup receive cnode\n");
          return;
        }

      cte_t *recv_slot = lookup_cap_slot (
          recv_cnode_root, receiver->ipc_buffer->receive_index,
          receiver->ipc_buffer->receive_depth, &err);
      if (err)
        {
          printf ("Failed to lookup receive slot\n");
          return;
        }

      copy_cap (recv_slot, cap, cap_rights_all);
    }

  if (sender->expects_reply)
    receiver->reply_to = sender;
}

static void
send_message_directly (struct tcb *receiver, message_info_t info, word_t badge,
                       bool resume_now)
{
  assert (receiver && "Receiver is NULL");
  assert (receiver->ipc_buffer && "Receiver has no IPC buffer");
  assert (this_tcb->ipc_buffer && "Sender has no IPC buffer");

  transfer_message (info, this_tcb, receiver, &receiver->saved_state, badge);

  if (resume_now)
    {
      receiver->state = TASK_STATE_RUNNABLE;
      switch_tcb (receiver);
    }
  else
    make_tcb_runnable (receiver);
}

static void
send_message_to_blocked_receiver (struct endpoint *e, message_info_t info,
                                  word_t badge, bool resume_now)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *receiver = CONTAINER_OF (next, struct tcb, send_receive_node);

  send_message_directly (receiver, info, badge, resume_now);
}

static void
queue_message_on_endpoint (struct endpoint *e, message_info_t info,
                           word_t badge, bool is_call)
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
receive_message_from_blocked_sender (struct endpoint *e)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *sender = CONTAINER_OF (next, struct tcb, send_receive_node);

  message_info_t info = (message_info_t)sender->ipc_buffer->tag;

  transfer_message (info, sender, this_tcb, this_tcb->current_user_frame,
                    sender->endpoint_badge);

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

static void
endpoint_nbsend (struct endpoint *e, word_t message_info, word_t badge)
{
  if (is_send_blocked (e))
    return;

  send_message_to_blocked_receiver (e, message_info, badge, false);
}

static message_info_t
endpoint_recv (struct endpoint *e)
{
  if (is_receive_blocked (e))
    queue_receiver_on_endpoint (e);
  else
    return receive_message_from_blocked_sender (e);
  return 0;
}

static void
endpoint_nbrecv (struct endpoint *e)
{
  if (is_receive_blocked (e))
    return;

  receive_message_from_blocked_sender (e);
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

  this_tcb->expects_reply = false;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, false);
}

void
invoke_endpoint_nbsend (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = false;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_nbsend (e, message_info, cap->cap.badge);
}

message_info_t
invoke_endpoint_recv (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  if (this_tcb->bound_notification && this_tcb->bound_notification->word)
    {
      this_tcb->current_user_frame->rdi = this_tcb->bound_notification->word;
      this_tcb->bound_notification->word = 0;
      return 0;
    }

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  return endpoint_recv (e);
}

message_info_t
invoke_endpoint_nbrecv (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  if (this_tcb->bound_notification && this_tcb->bound_notification->word)
    {
      this_tcb->current_user_frame->rdi = this_tcb->bound_notification->word;
      this_tcb->bound_notification->word = 0;
      return 0;
    }

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_nbrecv (e);
  return 0;
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
      debug_trap ();
      kill_tcb (this_tcb);
    }
  send_message_directly (receiver, message_info, 0, false);
}

message_info_t
invoke_reply_recv (cte_t *cap, word_t message_info)
{
  invoke_reply (message_info);
  return invoke_endpoint_recv (cap);
}
