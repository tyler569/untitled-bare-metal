#include "assert.h"
#include "stdio.h"
#include "sys/arch.h"
#include "sys/task.h"

static void
save_message_on_task (struct task *t, uintptr_t message)
{
  t->sending_message = message;
}

static void
switch_to_receive_fast (struct task *receiver, struct task *sender)
{
  debug_printf ("send -> receive fast\n");

  set_frame_return (receiver->saved_state, sender->sending_message);

  receiver->state = TASK_STATE_RUNNABLE;
  switch_task (receiver);
}

[[noreturn]] static void
queue_message (struct task *receiver, struct task *sender)
{
  debug_printf ("send -> queued message\n");

  spin_lock (&receiver->message_lock);

  append_to_list (&receiver->send_receive_pending,
                  &sender->send_receive_pending_node);
  sender->state = TASK_STATE_SENDING;

  spin_unlock (&receiver->message_lock);

  schedule ();

  UNREACHABLE ();
}

void
send_message (struct task *receiver, uintptr_t message)
{
  struct task *sender = this_cpu->current_task;
  assert (sender);

  save_message_on_task (sender, message);

  if (receiver->state == TASK_STATE_RECEIVING)
    switch_to_receive_fast (receiver, sender);
  else
    queue_message (receiver, sender);
}

void
receive_message ()
{
  struct task *receiver = this_cpu->current_task;
  assert (receiver);

  struct task *sender;

  spin_lock (&receiver->message_lock);

  if (is_list_empty (&receiver->send_receive_pending))
    {
      debug_printf ("receive -> no message\n");

      receiver->state = TASK_STATE_RECEIVING;
      save_task_state (receiver);

      spin_unlock (&receiver->message_lock);

      schedule ();
      UNREACHABLE ();
    }
  else
    {
      debug_printf ("receive -> got message\n");

      sender = CONTAINER_OF (pop_from_list (&receiver->send_receive_pending),
                             struct task, send_receive_pending_node);

      spin_unlock (&receiver->message_lock);

      set_frame_return (receiver->current_user_frame, sender->sending_message);
      make_task_runnable (sender);
    }
}
