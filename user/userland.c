#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#define PAGE_SIZE 4096
uint8_t __attribute__ ((aligned (PAGE_SIZE))) thread_stack[PAGE_SIZE * 4];
uint8_t __attribute__ ((aligned (PAGE_SIZE))) stack[PAGE_SIZE * 4];

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

void
do_untyped_retype (cptr_t src, word_t type, word_t size_bits, cptr_t root,
                   word_t index, word_t offset, word_t obj_count)
{
  message_info_t info = new_message_info (untyped_retype, 0, 7);
  set_mr (0, type);
  set_mr (1, size_bits);
  set_mr (2, root);   // root of destination cspace
  set_mr (3, index);  // index of destination cnode in destination cspace
  set_mr (4, 64);     // depth of destination cnode
  set_mr (5, offset); // offset in destination cnode
  set_mr (6, obj_count);

  call (src, info);

  word_t status = __ipc_buffer->tag;
  if (status != 0)
    {
      printf ("do_untyped_retype failed: %lu\n", get_message_label (status));
      exit ();
    }
}

void thread_entry (uintptr_t arg)
{
  printf ("Hello, World from userland thread! Arg is %lu\n", arg);
  exit ();
}

[[noreturn]] USED int
c_start (void *ipc_buffer, void *boot_info)
{
  __ipc_buffer = ipc_buffer;
  struct boot_info *bi = boot_info;

  printf ("Hello, World from userland; ipc buffer %p!\n", __ipc_buffer);

  printf ("Boot info: %p\n", bi);
  printf ("  .n_untypeds = %lu\n", bi->n_untypeds);

  // Send a message to the kernel
  message_info_t info = new_message_info (tcb_echo, 0, 0);
  call (init_cap_init_tcb, info);

  do_untyped_retype (4, cap_tcb, 0, init_cap_root_cnode, init_cap_root_cnode,
                     100, 1);

  info = new_message_info (cnode_debug_print, 0, 0);
  call (init_cap_root_cnode, info);

  info = new_message_info (tcb_configure, 2, 1);
  set_cap (0, 2); // cspace root
  set_cap (1, 3); // vspace root
  set_mr (0, 0); // ipc buffer
  call (100, info);

  frame_t frame;

  info = new_message_info (tcb_read_registers, 0, 1);
  set_mr (0, (word_t)&frame);
  call (init_cap_init_tcb, info);

  frame.rip = (uintptr_t)thread_entry;
  frame.rsp = (uintptr_t)thread_stack + sizeof (thread_stack);
  frame.rdi = 42;

  info = new_message_info (tcb_write_registers, 0, 1);
  set_mr (0, (word_t)&frame);
  call (100, info);

  printf ("Starting new userland thread\n");
  info = new_message_info (tcb_resume, 0, 0);
  call (100, info);

  info = new_message_info (tcb_echo, 0, 0);
  call (100, info);

  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
