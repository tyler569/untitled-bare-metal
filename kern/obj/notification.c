#include "kern/obj/notification.h"
#include "kern/cap.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"

static void
signal_waiting_receiver (struct tcb *receiver, word_t badge)
{
  message_info_t tag = new_message_info (no_error, 0, 0, 0);

  receiver->ipc_buffer->tag = tag;
  receiver->saved_state.rax = tag;
  receiver->saved_state.rdi = badge;

  receiver->state = TASK_STATE_RUNNABLE;
  switch_tcb (receiver);
}

static void
signal_bound_receiver_waiting_on_something_else (struct tcb *receiver,
                                                 word_t badge)
{
  remove_from_list (&receiver->send_receive_node);
  signal_waiting_receiver (receiver, badge);
}

static void
queue_receiver_on_notification (struct notification *n)
{
  append_to_list (&this_tcb->send_receive_node, &n->list);
  this_tcb->state = TASK_STATE_RECEIVING;

  schedule ();
}

static void
maybe_init_notification (struct notification *n)
{
  if (n->list.next)
    return;

  init_list (&n->list);
}

void
notification_signal (struct notification *n, word_t badge)
{
  maybe_init_notification (n);

  n->word |= badge;
  word_t nfn_word = n->word;

  if (is_list_empty (&n->list) && n->bound_tcb
      && n->bound_tcb->state == TASK_STATE_RECEIVING)
    {
      n->word = 0;
      signal_bound_receiver_waiting_on_something_else (n->bound_tcb, nfn_word);
      return;
    }

  if (is_list_empty (&n->list))
    return;

  n->word = 0;

  struct tcb *tcb
      = CONTAINER_OF (first_item (&n->list), struct tcb, send_receive_node);
  remove_from_list (&tcb->send_receive_node);

  signal_waiting_receiver (tcb, nfn_word);
}

void
invoke_notification_send (cte_t *cap)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *n = cap_ptr (cap);
  maybe_init_notification (n);
  notification_signal (n, cap->cap.badge);
}

message_info_t
invoke_notification_recv (cte_t *cap)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *n = cap_ptr (cap);
  maybe_init_notification (n);

  if (n->bound_tcb && n->bound_tcb != this_tcb)
    {
      kill_tcb (this_tcb);
      schedule ();
      return new_message_info (invalid_argument, 0, 0, 0);
    }

  if (n->word == 0)
    {
      queue_receiver_on_notification (n);
      return 0;
    }

  n->word = 0;

  return return_ipc (no_error, 0);
}
