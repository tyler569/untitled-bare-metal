#include "string.h"
#include "sys/arch.h"
#include "sys/mem.h"
#include "x86_64.h"

void
save_frame_on_task (frame_t *f)
{
  if (this_cpu->current_task && f->cs == USER_CS)
    this_cpu->current_task->current_interrupt_frame = f;
}

void
clear_frame_on_task (frame_t *f)
{
  if (!this_cpu->current_task)
    return;
  if (this_cpu->current_task->current_interrupt_frame != f)
    return;
  this_cpu->current_task->current_interrupt_frame = nullptr;
}

frame_t *
new_user_frame (uintptr_t rip, uintptr_t rsp)
{
  frame_t *f = kmem_alloc (sizeof (frame_t));
  memset (f, 0, sizeof (frame_t));

  f->cs = USER_CS;
  f->ss = USER_SS;
  f->rip = rip;
  f->rsp = rsp;
  return f;
}

void
copy_frame (frame_t *dst, frame_t *src)
{
  memcpy (dst, src, sizeof (frame_t));
}
