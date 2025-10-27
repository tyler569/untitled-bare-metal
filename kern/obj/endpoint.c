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

static message_info_t
transfer_message (struct tcb *sender, struct tcb *receiver, word_t badge,
                  message_info_t tag)
{
  word_t size = get_message_length (tag) * sizeof (word_t);
  memcpy (receiver->ipc_buffer->msg, sender->ipc_buffer->msg, size);

  receiver->ipc_buffer->tag = tag;
  receiver->ipc_buffer->sender_badge = badge;

  word_t transfer_cap = get_message_extra_caps (tag);
  if (transfer_cap)
    {
      cptr_t send_cptr = sender->ipc_buffer->caps_or_badges[0];
      cte_t *cap;
      error_t err
          = lookup_cap_slot_raw (&sender->cspace_root, send_cptr, 64, &cap);
      if (err != no_error)
        {
          // Silently drop cap transfer - clear extra_caps in receiver's tag
          receiver->ipc_buffer->tag.extra_caps = 0;
          goto finish;
        }

      cte_t *recv_cnode_root;
      err = lookup_cap_slot_raw (&receiver->cspace_root,
                                 receiver->ipc_buffer->receive_cnode, 64,
                                 &recv_cnode_root);
      if (err != no_error)
        {
          receiver->ipc_buffer->tag.extra_caps = 0;
          goto finish;
        }

      cte_t *recv_slot;
      err = lookup_cap_slot_raw (
          recv_cnode_root, receiver->ipc_buffer->receive_index,
          receiver->ipc_buffer->receive_depth, &recv_slot);
      if (err != no_error)
        {
          receiver->ipc_buffer->tag.extra_caps = 0;
          goto finish;
        }

      copy_cap (recv_slot, cap, cap_rights_all);
    }

finish:
  if (sender->expects_reply)
    receiver->reply_to = sender;

  return receiver->ipc_buffer->tag;
}

static void
send_message_directly (struct tcb *receiver, word_t badge, message_info_t tag,
                       bool resume_now)
{
  assert (receiver && "Receiver is NULL");
  assert (receiver->ipc_buffer && "Receiver has no IPC buffer");
  assert (this_tcb->ipc_buffer && "Sender has no IPC buffer");

  transfer_message (this_tcb, receiver, badge, tag);

  if (resume_now)
    {
      receiver->state = TASK_STATE_RUNNABLE;
      switch_tcb (receiver);
    }
  else
    make_tcb_runnable (receiver);
}

static void
send_message_to_blocked_receiver (struct endpoint *e, word_t badge,
                                  message_info_t tag, bool resume_now)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *receiver = CONTAINER_OF (next, struct tcb, send_receive_node);

  send_message_directly (receiver, badge, tag, resume_now);
}

static void
queue_message_on_endpoint (struct endpoint *e, word_t badge,
                           message_info_t tag, bool is_call)
{
  this_tcb->ipc_buffer->tag = tag;
  append_to_list (&this_tcb->send_receive_node, &e->list);

  if (is_call)
    this_tcb->state = TASK_STATE_CALLING;
  else
    this_tcb->state = TASK_STATE_SENDING;

  this_tcb->endpoint_badge = badge;

  schedule ();
}

static message_info_t
receive_message_from_blocked_sender (struct endpoint *e)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *sender = CONTAINER_OF (next, struct tcb, send_receive_node);

  message_info_t tag = sender->ipc_buffer->tag;
  message_info_t result
      = transfer_message (sender, this_tcb, sender->endpoint_badge, tag);

  if (sender->expects_reply)
    this_tcb->reply_to = sender;
  else
    make_tcb_runnable (sender);

  return result;
}

static message_info_t
queue_receiver_on_endpoint (struct endpoint *e)
{
  append_to_list (&this_tcb->send_receive_node, &e->list);
  this_tcb->state = TASK_STATE_RECEIVING;
  schedule ();
  return msg_noreturn ();
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
endpoint_send (struct endpoint *e, word_t badge, message_info_t tag,
               bool is_call)
{
  if (is_send_blocked (e))
    queue_message_on_endpoint (e, badge, tag, is_call);
  else
    send_message_to_blocked_receiver (e, badge, tag, true);
}

static void
endpoint_nbsend (struct endpoint *e, word_t badge, message_info_t tag)
{
  if (is_send_blocked (e))
    return;

  send_message_to_blocked_receiver (e, badge, tag, false);
}

static message_info_t
endpoint_recv (struct endpoint *e)
{
  if (is_receive_blocked (e))
    return queue_receiver_on_endpoint (e);
  else
    return receive_message_from_blocked_sender (e);
}

static message_info_t
endpoint_nbrecv (struct endpoint *e)
{
  if (is_receive_blocked (e))
    return msg_ok (0);

  return receive_message_from_blocked_sender (e);
}

static void
maybe_init_endpoint (struct endpoint *e)
{
  if (e->list.next)
    return;

  init_list (&e->list);
}

void
invoke_endpoint_send (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = false;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);

  message_info_t tag = this_tcb->ipc_buffer->tag;
  endpoint_send (e, cap->cap.badge, tag, false);
}

void
invoke_endpoint_nbsend (cte_t *cap)
{
  printf ("nbsend: %s\n", cap_type_string (cap));
  panic ("how did we get here\n");

  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = false;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);

  message_info_t tag = this_tcb->ipc_buffer->tag;
  endpoint_nbsend (e, cap->cap.badge, tag);
}

bool
handle_recv_with_pending_notification ()
{
  if (this_tcb->bound_notification && this_tcb->bound_notification->word)
    {
      this_tcb->ipc_buffer->sender_badge = this_tcb->bound_notification->word;
      this_tcb->bound_notification->word = 0;
      return true;
    }
  else
    return false;
}

message_info_t
invoke_endpoint_recv (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  if (handle_recv_with_pending_notification ())
    return msg_ok (0);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  return endpoint_recv (e);
}

message_info_t
invoke_endpoint_nbrecv (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  if (handle_recv_with_pending_notification ())
    return msg_ok (0);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  return endpoint_nbrecv (e);
}

void
invoke_endpoint_call (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = true;
  this_tcb->state = TASK_STATE_CALLING;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);

  message_info_t tag = this_tcb->ipc_buffer->tag;
  endpoint_send (e, cap->cap.badge, tag, true);
}

void
invoke_reply ()
{
  // TODO: this should use tcb->reply capability

  struct tcb *receiver = this_tcb->reply_to;
  if (!receiver || receiver->state != TASK_STATE_CALLING)
    {
      printf ("Failed to deliver reply!\n");
      debug_trap ();
      kill_tcb (this_tcb);
    }

  message_info_t tag = this_tcb->ipc_buffer->tag;
  send_message_directly (receiver, 0, tag, false);
}

message_info_t
invoke_reply_recv (cte_t *cap)
{
  invoke_reply ();
  return invoke_endpoint_recv (cap);
}
