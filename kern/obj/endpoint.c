#include "kern/obj/endpoint.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/tcb.h"

[[noreturn]] static void
send_message_to_blocked_receiver (struct tcb *receiver, word_t info,
                                  word_t badge)
{
  (void)badge;

  remove_from_list (&receiver->send_receive_node);

  for (size_t i = 0; i < get_message_length (info); i++)
    {
      word_t word = get_mr (i);
      receiver->ipc_buffer->msg[i] = word;
    }

  // receiver->ipc_buffer->msg_info = info;
  // receiver->ipc_buffer->badge = badge;

  make_tcb_runnable (receiver);
  switch_tcb (receiver);
}

[[noreturn]] static void
queue_message_on_endpoint (struct endpoint *e, word_t info)
{
  struct tcb *tcb
      = CONTAINER_OF (first_item (&e->list), struct tcb, send_receive_node);

  (void)tcb;

  append_to_list (&this_tcb->send_receive_node, &e->list);
  if (this_tcb->ipc_buffer)
    this_tcb->ipc_buffer->tag = info;
  this_tcb->state = TASK_STATE_SENDING;
  schedule ();
  unreachable ();
}

[[noreturn]] static void
endpoint_send (struct endpoint *e, word_t message_info, word_t badge)
{
  if (is_list_empty (&e->list))
    {
      // block the sender on the queue
      append_to_list (&this_tcb->send_receive_node, &e->list);
      this_tcb->state = TASK_STATE_SENDING;
      schedule ();
      unreachable ();
    }
  else
    {
      struct tcb *tcb = CONTAINER_OF (first_item (&e->list), struct tcb,
                                      send_receive_node);
      bool send_blocked = tcb->state == TASK_STATE_SENDING;
      bool recv_blocked = tcb->state == TASK_STATE_RECEIVING;

      if (send_blocked)
        queue_message_on_endpoint (e, message_info);
      else if (recv_blocked)
        send_message_to_blocked_receiver (tcb, message_info, badge);
      else
        assert (false && "Neither send nor receive blocked");
    }
}

[[noreturn]] error_t
invoke_endpoint_send (cap_t cap, word_t message_info)
{
  assert (cap_type (cap) == cap_endpoint);

  struct endpoint *e = cap_ptr (cap);
  endpoint_send (e, message_info, cap.words[1]);
  unreachable ();
}
