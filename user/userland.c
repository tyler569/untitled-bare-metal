#include "stdint.h"
#include "stdio.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

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

struct ipc_buffer *__ipc_buffer;

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
send (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_send, cap, info);
}

void
call (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_call, cap, info);
}

[[noreturn]] static inline void
exit ()
{
  _syscall0 (0);
  UNREACHABLE ();
}

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (1, (uintptr_t)str, len);
  return (long)len;
}

[[noreturn]] USED int
_start (uintptr_t arg)
{
  __ipc_buffer = (void *)arg;
  printf ("Hello, World from userland; ipc buffer %p!\n", __ipc_buffer);

  // Send a message to the kernel
  message_info_t info = new_message_info (0, 0, 0);
  set_mr (0, 0xdeadbeef);
  call (0, info);

  exit ();
}
