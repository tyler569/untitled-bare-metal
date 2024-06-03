#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#define PAGE_SIZE 4096
uint8_t __attribute__ ((aligned (PAGE_SIZE))) thread_stack[PAGE_SIZE * 4];
uint8_t __attribute__ ((aligned (PAGE_SIZE))) stack[PAGE_SIZE * 4];

__thread struct ipc_buffer *__ipc_buffer;

struct thread_local_storage
{
  uintptr_t ipc_buffer;
  struct thread_local_storage *self;
};

struct thread_local_storage tls1 = { 0, &tls1 };
struct thread_local_storage tls2 = { 0, &tls2 };

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};
typedef struct frame frame_t;

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

#include "sys/user_method_stubs.h"

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

[[noreturn]] void
thread_entry (void *ipc_buffer, uintptr_t arg)
{
  asm volatile ("wrfsbase %0" ::"r"(&tls2.self));
  __ipc_buffer = ipc_buffer;
  printf ("Hello, World from userland thread! Arg is %lu\n", arg);

  message_info_t info = new_message_info (0, 0, 0, 1);
  set_mr (0, 42);
  send (101, info);

  exit ();
}

[[noreturn]] int
c_start (void *ipc_buffer, void *boot_info)
{
  asm volatile ("wrfsbase %0" ::"r"(&tls1.self));
  __ipc_buffer = ipc_buffer;
  struct boot_info *bi = boot_info;

  printf ("Hello, World from userland; ipc buffer %p!\n", __ipc_buffer);

  printf ("Boot info: %p\n", bi);
  printf ("  .n_untypeds = %lu\n", bi->n_untypeds);

  untyped_retype (4, cap_tcb, 0, init_cap_root_cnode, init_cap_root_cnode, 64,
                  100, 1);
  untyped_retype (4, cap_endpoint, 0, init_cap_root_cnode, init_cap_root_cnode,
                  64, 101, 1);
  // cnode_debug_print (init_cap_root_cnode);

  tcb_configure (100, 0, init_cap_root_cnode, 0, init_cap_init_vspace, 0,
				 (word_t)ipc_buffer + 1024, 0);

  frame_t frame;
  tcb_read_registers (init_cap_init_tcb, false, 0, 0, &frame);

  frame.rip = (uintptr_t)thread_entry;
  frame.rsp = (uintptr_t)thread_stack + sizeof (thread_stack);
  frame.rdi = (uintptr_t)ipc_buffer + 1024;
  frame.rsi = 42;

  tcb_write_registers (100, false, 0, 0, &frame);

  printf ("Starting new userland thread\n");
  tcb_resume (100);

  word_t badge;
  message_info_t resp = recv (101, &badge);
  printf ("Received message on endpoint 101, badge %lu\n", badge);
  printf ("Message info: %#lx, mr0: %lu\n", resp, get_mr (0));

  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
