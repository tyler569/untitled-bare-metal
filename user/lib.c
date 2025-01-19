extern thread_local struct ipc_buffer *__ipc_buffer;

static uintptr_t
_syscall0 (int syscall_num)
{
  uintptr_t ret;
  asm volatile ("syscall" : "=a"(ret) : "0"(syscall_num) : "rcx", "r11");
  return ret;
}

static uintptr_t
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1), "S"(a2)
                : "rcx", "r11");
  return ret;
}

static uintptr_t
_syscall22 (int syscall_num, uintptr_t a1, uintptr_t a2, uintptr_t *out)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret), "=D"(*out)
                : "0"(syscall_num), "1"(a1), "S"(a2)
                : "rcx", "r11");
  return ret;
}

void
set_mr (word_t i, word_t val)
{
  __ipc_buffer->msg[i] = val;
}

word_t
get_mr (word_t i)
{
  return __ipc_buffer->msg[i];
}

void
set_cap (word_t i, word_t val)
{
  __ipc_buffer->caps_or_badges[i] = val;
}

word_t
get_cap (word_t i)
{
  return __ipc_buffer->caps_or_badges[i];
}

void
send (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_send, cap, info);
}

message_info_t
call (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_call, cap, info, sender);
}

message_info_t
recv (cptr_t cap, word_t *sender)
{
  return _syscall22 (sys_recv, cap, 0, sender);
}

message_info_t
reply (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  return _syscall2 (sys_reply, cap, info);
}

message_info_t
reply_recv (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_replyrecv, cap, info, sender);
}

void
yield ()
{
  _syscall0 (sys_yield);
}

#include "sys/user_method_stubs.h"

[[noreturn]] static inline void
exit ()
{
  _syscall0 (0);
  unreachable ();
}

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};
typedef struct frame frame_t;

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (sys_debug_write, (uintptr_t)str, len);
  return (long)len;
}
