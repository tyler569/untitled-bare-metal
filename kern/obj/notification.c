#include "kern/obj/notification.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"
#include "string.h"

static void
signal_waiting_receiver (struct tcb *receiver, word_t badge)
{
  message_info_t tag = new_message_info (no_error, 0, 0, 0);

  receiver->ipc_buffer->tag = tag;
  receiver->saved_state.rax = tag;
  receiver->saved_state.rdi = badge;

  receiver->state = TASK_STATE_RUNNABLE;
  switch_tcb (receiver);
  unreachable ();
}

static void
signal_bound_receiver_waiting_on_something_else (struct tcb *receiver,
                                                 word_t badge)
{
  remove_from_list (&receiver->send_receive_node);
  signal_waiting_receiver (receiver, badge);
}

[[noreturn]] static void
queue_receiver_on_notification (struct notification *n)
{
  append_to_list (&this_tcb->send_receive_node, &n->list);
  this_tcb->state = TASK_STATE_RECEIVING;

  schedule ();
  unreachable ();
}

static void
notification_signal (struct notification *n, word_t badge)
{
  n->word |= badge;

  if (is_list_empty (&n->list))
    {
      if (n->bound_tcb && n->bound_tcb->state == TASK_STATE_RECEIVING)
        signal_bound_receiver_waiting_on_something_else (n->bound_tcb, badge);
      printf ("RETURNING OUT OF SIGNAL\n");
      return;
    }

  word_t nfn_word = n->word;
  n->word = 0;

  struct tcb *tcb
      = CONTAINER_OF (first_item (&n->list), struct tcb, send_receive_node);
  remove_from_list (&tcb->send_receive_node);

  signal_waiting_receiver (tcb, nfn_word);
}

static void
maybe_init_notification (struct notification *n)
{
  printf ("n: %p\n", n);
  assert (n);

  if (n->list.next)
    return;

  n->word = 0;
  n->bound_tcb = nullptr;
  init_list (&n->list);
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
invoke_notification_recv (cte_t *cap, word_t *nfn_word)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *n = cap_ptr (cap);
  maybe_init_notification (n);

  if (n->bound_tcb && n->bound_tcb != this_tcb)
    kill_tcb (this_tcb);

  if (n->word == 0)
    queue_receiver_on_notification (n);

  *nfn_word = n->word;
  n->word = 0;

  return return_ipc (no_error, 0);
}
