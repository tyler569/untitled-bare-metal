#pragma once
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

extern struct ipc_buffer *__ipc_buffer;

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};
typedef struct frame frame_t;

void cptr_alloc_init (struct boot_info *bi);
cptr_t cptr_alloc ();
cptr_t cptr_alloc_range (size_t n);
void cptr_free (cptr_t cptr);

struct buffer
{
  cptr_t cptr_base;
  size_t pages;
};
typedef struct buffer buffer_t;

buffer_t create_buffer (cptr_t untyped, size_t pages);
int map_page (cptr_t untyped, cptr_t vspace, cptr_t page, uintptr_t addr);
int map_buffer (cptr_t untyped, cptr_t vspace, buffer_t buffer,
                uintptr_t addr);

int create_process (void *elf_data, size_t elf_size, cptr_t untyped,
                    cptr_t our_vspace, cptr_t *tcb, cptr_t *cspace);

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

static inline void
set_mr (word_t i, word_t val)
{
  __ipc_buffer->msg[i] = val;
}

static inline word_t
get_mr (word_t i)
{
  return __ipc_buffer->msg[i];
}

static inline void
set_cap (word_t i, word_t val)
{
  __ipc_buffer->caps_or_badges[i] = val;
}

static inline word_t
get_cap (word_t i)
{
  return __ipc_buffer->caps_or_badges[i];
}

static inline void
send (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_send, cap, info);
}

static inline message_info_t
call (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_call, cap, info, sender);
}

static inline message_info_t
recv (cptr_t cap, word_t *sender)
{
  return _syscall22 (sys_recv, cap, 0, sender);
}

static inline message_info_t
reply (message_info_t info)
{
  __ipc_buffer->tag = info;
  return _syscall2 (sys_reply, info, 0);
}

static inline message_info_t
reply_recv (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_replyrecv, cap, info, sender);
}

static inline void
yield ()
{
  _syscall0 (sys_yield);
}

[[noreturn]] static inline void
exit ()
{
  _syscall0 (0);
  unreachable ();
}

inline long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (sys_debug_write, (uintptr_t)str, len);
  return (long)len;
}

#include "sys/user_method_stubs.h"

static inline cptr_t
allocate (cptr_t untyped, word_t type, size_t n)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, 0, init_cap_root_cnode, init_cap_root_cnode,
                  64, cptr, n);
  return cptr;
}
