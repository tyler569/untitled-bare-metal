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
    return;

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
  if (n->list.next)
    return;

  n->word = 0;
  init_list (&n->list);
}

[[noreturn]] void
invoke_notification_send (cte_t *cap)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *n = cap_ptr (cap);
  maybe_init_notification (n);
  notification_signal (n, cap->cap.badge);
  unreachable ();
}

message_info_t
invoke_notification_recv (cte_t *cap, word_t *nfn_word)
{
  assert (cap_type (cap) == cap_notification);

  struct notification *n = cap_ptr (cap);
  maybe_init_notification (n);

  if (n->word == 0)
    queue_receiver_on_notification (n);

  *nfn_word = n->word;
  n->word = 0;

  return return_ipc (no_error, 0);
}
