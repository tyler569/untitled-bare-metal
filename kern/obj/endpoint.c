#include "kern/obj/endpoint.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"

[[noreturn]] static void
send_message_to_blocked_receiver (struct tcb *receiver, word_t info,
                                  word_t badge)
{
  (void)badge;

  word_t tmp;
  for (size_t i = 0; i < get_message_length (info); i++)
    {
      tmp = this_tcb->ipc_buffer->msg[i];
      receiver->ipc_buffer->msg[i] = tmp;
    }

  receiver->ipc_buffer->tag = info;
  receiver->saved_state.rax = info;
  receiver->saved_state.rdi = badge;

  if (this_tcb->expects_reply)
    receiver->reply_to = this_tcb;

  receiver->state = TASK_STATE_RUNNABLE;
  switch_tcb (receiver);
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

static void
receive_message_from_blocked_sender (struct endpoint *e, word_t *info,
                                     word_t *badge)
{
  struct list_head *next = pop_from_list (&e->list);
  struct tcb *sender = CONTAINER_OF (next, struct tcb, send_receive_node);

  *info = sender->ipc_buffer->tag;
  *badge = sender->endpoint_badge;

  word_t tmp;
  for (size_t i = 0; i < get_message_length (*info); i++)
    {
      tmp = sender->ipc_buffer->msg[i];
      this_tcb->ipc_buffer->msg[i] = tmp;
    }

  make_tcb_runnable (sender);
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
          send_message_to_blocked_receiver (tcb, message_info, badge);
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

[[noreturn]] error_t
invoke_endpoint_send (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, false);
  unreachable ();
}

error_t
invoke_endpoint_recv (cte_t *cap)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  if (is_list_empty (&e->list))
    {
      queue_receiver_on_endpoint (e);
    }
  else
    {
      receive_message_from_blocked_sender (e,
                                           &this_tcb->current_user_frame->rax,
                                           &this_tcb->current_user_frame->rdi);
      return no_error;
    }
}

[[noreturn]] error_t
invoke_endpoint_call (cte_t *cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  this_tcb->expects_reply = true;

  struct endpoint *e = cap_ptr (cap);
  maybe_init_endpoint (e);
  endpoint_send (e, message_info, cap->cap.badge, true);
  unreachable ();
}

error_t
invoke_reply (cte_t *, word_t message_info)
{
  struct tcb *receiver = this_tcb->reply_to;
  if (!receiver || receiver->state != TASK_STATE_CALLING)
    {
      return_ipc (illegal_operation, 0);
      return illegal_operation;
    }
  send_message_to_blocked_receiver (receiver, message_info, 0);
}
