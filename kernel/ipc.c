#include "kern/ipc.h"
#include "kern/task.h"
#include "kern/per_cpu.h"

word_t
get_ipc_info ()
{
  if (this_task && this_task->ipc_buffer)
    return this_task->ipc_buffer->tag;
  else
    return 0;
}

word_t
get_ipc_label ()
{
  return get_message_label (get_ipc_info ());
}

word_t
get_ipc_extra_caps ()
{
  return get_message_extra_caps (get_ipc_info ());
}

word_t
get_ipc_length ()
{
  return get_message_length (get_ipc_info ());
}

word_t
get_mr (word_t i)
{
  if (this_task && this_task->ipc_buffer)
    return this_task->ipc_buffer->msg[i];
  else
    return 0;
}

word_t
get_extra_cap (word_t i)
{
  if (this_task && this_task->ipc_buffer)
	return this_task->ipc_buffer->caps_or_badges[i];
  else
	return 0;
}

void
set_mr (word_t i, word_t v)
{
  if (this_task && this_task->ipc_buffer)
    this_task->ipc_buffer->msg[i] = v;
}

void
set_ipc_info (message_info_t tag)
{
  if (this_task && this_task->ipc_buffer)
    this_task->ipc_buffer->tag = tag;
}

void
return_ipc_error (word_t err, word_t registers)
{
  set_ipc_info (new_message_info (err, 0, registers));
}
