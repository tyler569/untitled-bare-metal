#include "assert.h"
#include "stdio.h"
#include "string.h"
#include "sys/arch.h"
#include "sys/mem.h"
#include "x86_64.h"

void
save_frame_on_task (frame_t *f)
{
  if (this_task && f->cs == USER_CS)
    this_task->current_user_frame = f;
}

void
clear_frame_on_task (frame_t *f)
{
  if (!this_task)
    return;
  if (this_task->current_user_frame != f)
    return;
  this_task->current_user_frame = nullptr;
}

void
new_user_frame (frame_t *f, uintptr_t rip, uintptr_t rsp)
{
  memset (f, 0, sizeof (frame_t));

  f->cs = USER_CS;
  f->ss = USER_SS;
  f->rip = rip;
  f->rsp = rsp;
}

void
copy_frame (frame_t *dst, frame_t *src)
{
  memcpy (dst, src, sizeof (frame_t));
}

uintptr_t
get_frame_arg (frame_t *f, int n)
{
  switch (n)
    {
    case 0:
      return f->rdi;
    case 1:
      return f->rsi;
    case 2:
      return f->rdx;
    case 3:
      return f->rcx;
    case 4:
      return f->r8;
    case 5:
      return f->r9;
    default:
      panic ("get_frame_arg: invalid argument number %d", n);
    }
}

uintptr_t
get_frame_syscall_arg (frame_t *f, int n)
{
  if (n == 3)
    return f->r10;
  else
    return get_frame_arg (f, n);
}

void
set_frame_arg (frame_t *f, int n, uintptr_t val)
{
  switch (n)
    {
    case 0:
      f->rdi = val;
      break;
    case 1:
      f->rsi = val;
      break;
    case 2:
      f->rdx = val;
      break;
    case 3:
      f->rcx = val;
      break;
    case 4:
      f->r8 = val;
      break;
    case 5:
      f->r9 = val;
      break;
    default:
      panic ("set_frame_arg: invalid argument number %d", n);
    }
}

void
set_frame_return (frame_t *f, uintptr_t val)
{
  f->rax = val;
}
