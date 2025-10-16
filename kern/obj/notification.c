#include "kern/obj/notification.h"
#include "kern/cap.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"

static void
signal_waiting_receiver (struct tcb *receiver, word_t badge)
{
  message_info_t tag = new_message_info (no_error, 0, 0, 0);

  receiver->ipc_buffer->tag = tag;
  receiver->ipc_buffer->sender_badge = badge;

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
queue_receiver_on_notification (struct notification *nfn)
{
  append_to_list (&this_tcb->send_receive_node, &nfn->list);
  this_tcb->state = TASK_STATE_RECEIVING;

  schedule ();
}

static void
maybe_init_notification (struct notification *nfn)
{
  if (nfn->list.next)
    return;

  init_list (&nfn->list);
}

void
notification_signal (struct notification *nfn, word_t badge)
{
  maybe_init_notification (nfn);

  nfn->word |= badge;
  word_t nfn_word = nfn->word;

  if (is_list_empty (&nfn->list) && nfn->bound_tcb
      && nfn->bound_tcb->state == TASK_STATE_RECEIVING)
    {
      nfn->word = 0;
      signal_bound_receiver_waiting_on_something_else (nfn->bound_tcb,
                                                       nfn_word);
      return;
    }

  if (is_list_empty (&nfn->list))
    return;

  nfn->word = 0;

  struct tcb *tcb
      = CONTAINER_OF (first_item (&nfn->list), struct tcb, send_receive_node);
  remove_from_list (&tcb->send_receive_node);

  signal_waiting_receiver (tcb, nfn_word);
}

void
invoke_notification_send (cte_t *cap)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *nfn = cap_ptr (cap);
  maybe_init_notification (nfn);
  notification_signal (nfn, cap->cap.badge);
}

error_t
invoke_notification_recv (cte_t *cap)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *nfn = cap_ptr (cap);
  maybe_init_notification (nfn);

  if (nfn->bound_tcb && nfn->bound_tcb != this_tcb)
    {
      err_printf (
          "killing thread attempted to wait on bound notification\nfn");
      kill_tcb (this_tcb);
      schedule ();
    }

  if (nfn->word == 0)
    {
      queue_receiver_on_notification (nfn);
      return 0;
    }

  nfn->word = 0;

  return ipc_ok (0);
}
