#include "kern/obj/endpoint.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"
#include "string.h"

static void
send_message_to_blocked_receiver (struct tcb *receiver, word_t info,
                                  word_t badge, bool resume_now)
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
      unreachable ();
    }
  else
    make_tcb_runnable (receiver);
}

[[noreturn]] static void
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
  unreachable ();
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

[[noreturn]] static void
queue_receiver_on_endpoint (struct endpoint *e)
{
  append_to_list (&this_tcb->send_receive_node, &e->list);
  this_tcb->state = TASK_STATE_RECEIVING;
  schedule ();
  unreachable ();
}

[[noreturn]] static void
endpoint_send (struct endpoint *e, word_t message_info, word_t badge,
               bool is_call)
{
  if (is_list_empty (&e->list))
    queue_message_on_endpoint (e, message_info, badge, is_call);
  else
    {
      struct tcb *tcb = CONTAINER_OF (first_item (&e->list), struct tcb,
                                      send_receive_node);
      bool send_blocked = tcb->state == TASK_STATE_SENDING
                          || tcb->state == TASK_STATE_CALLING;
      bool recv_blocked = tcb->state == TASK_STATE_RECEIVING;

      if (send_blocked)
        queue_message_on_endpoint (e, message_info, badge, is_call);
      else if (recv_blocked)
        {
          remove_from_list (&tcb->send_receive_node);
          send_message_to_blocked_receiver (tcb, message_info, badge, true);
          unreachable ();
        }
      else
        assert (false && "Neither send nor receive blocked");
    }
}

static void
maybe_init_endpoint (struct endpoint *e)
{
  if (e->list.next)
    return;

  init_list (&e->list);
}

[[noreturn]] void
invoke_endpoint_send (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, false);
  unreachable ();
}

message_info_t
invoke_endpoint_recv (cte_t *cap, word_t *sender)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  if (is_list_empty (&e->list))
    queue_receiver_on_endpoint (e);
  else
    return receive_message_from_blocked_sender (e, sender);
}

[[noreturn]] message_info_t
invoke_endpoint_call (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = true;
  this_tcb->state = TASK_STATE_CALLING;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, true);
  unreachable ();
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
  send_message_to_blocked_receiver (receiver, message_info, 0, false);
}

message_info_t
invoke_reply_recv (cte_t *cap, word_t message_info, word_t *sender)
{
  invoke_reply (message_info);
  return invoke_endpoint_recv (cap, sender);
}
