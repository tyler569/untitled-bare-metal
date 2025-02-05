#include "stddef.h"
#include "stdint.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

#include "lib.h"

static inline uintptr_t
_syscall0 (int syscall_num)
{
  uintptr_t ret;
  asm volatile ("syscall" : "=a"(ret) : "0"(syscall_num) : "rcx", "r11");
  return ret;
}

static inline uintptr_t
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1), "S"(a2)
                : "rcx", "r11");
  return ret;
}

static inline uintptr_t
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
send (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_send, cap, info);
}

void
signal (cptr_t cap)
{
  _syscall2 (sys_send, cap, 0);
}

message_info_t
call (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_call, cap, info, sender);
}

message_info_t
__call_kernel (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  return _syscall2 (sys_call, cap, info);
}

message_info_t
recv (cptr_t cap, word_t *sender)
{
  return _syscall22 (sys_recv, cap, 0, sender);
}

void
wait (cptr_t cap, word_t *nfn_word)
{
  _syscall22 (sys_recv, cap, 0, nfn_word);
}

message_info_t
reply (message_info_t info)
{
  __ipc_buffer->tag = info;
  return _syscall2 (sys_reply, info, 0);
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

[[noreturn]] void
exit (int)
{
  _syscall0 (0);
  unreachable ();
}

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (sys_debug_write, (uintptr_t)str, len);
  return (long)len;
}

cptr_t
allocate (cptr_t untyped, word_t type, size_t n)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, 0, init_cap_root_cnode, init_cap_root_cnode,
                  64, cptr, n);
  return cptr;
}
