#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#define PAGE_SIZE 4096
uint8_t __attribute__ ((aligned (PAGE_SIZE))) thread_stack[PAGE_SIZE * 4];
uint8_t __attribute__ ((aligned (PAGE_SIZE))) stack[PAGE_SIZE * 4];

thread_local struct ipc_buffer *__ipc_buffer;

struct thread_local_storage
{
  char data[512 - 8];
  void *self;
};

struct thread_local_storage tls1 = { {}, (void *)&tls1.self };
struct thread_local_storage tls2 = { {}, (void *)&tls2.self };

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
  UNREACHABLE ();
}

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (sys_debug_write, (uintptr_t)str, len);
  return (long)len;
}

[[noreturn]] void
thread_entry (void *ipc_buffer, uintptr_t arg)
{
  asm volatile ("wrfsbase %0" ::"r"(&tls2.self));
  __ipc_buffer = ipc_buffer;
  printf ("Hello, World from userland thread! Arg is %lu\n", arg);

  message_info_t info = new_message_info (0, 0, 0, 1);
  for (word_t i = 0; i < 10; i++)
    {
      set_mr (0, i);
      send (101, info);
    }

  info = new_message_info (1, 0, 0, 0);
  send (101, info);

  exit ();
}

extern char __executable_start;

struct boot_info *bi = nullptr;
int port_cap = 0;

void
issue_port_cap()
{
  if (port_cap != 0)
    {
      return;
    }

  int err = x86_64_io_port_control_issue (
                                          init_cap_io_port_control,
                                          0x0,
                                          0xffff,
                                          init_cap_root_cnode,
                                          102,
                                          64
                                         );
  if (err != 0)
    {
      printf ("Failed to issue port_e9_cap: %s\n", error_string(err));
      return;
    }

  port_cap = 102;
}

void
print_to_e9(const char *string)
{
  issue_port_cap ();

  if (port_cap == 0)
    {
      return;
    }

  for (const char *c = string; *c; c++)
    {
      x86_64_io_port_out8 (port_cap, 0xe9, *c);
    }
}

[[noreturn]] int
c_start (void *ipc_buffer, void *boot_info)
{
  asm volatile ("wrfsbase %0" ::"r"(&tls1.self));
  __ipc_buffer = ipc_buffer;

  tcb_set_tls_base (init_cap_init_tcb, (uintptr_t)&tls1.self);

  bi = boot_info;

  printf ("Hello, World from userland; ipc buffer %p!\n", __ipc_buffer);
  printf ("executable_start: %p\n", &__executable_start);

  print_to_e9 ("[E9] Hello World!\n");

  printf ("Boot info: %p\n", bi);
  printf ("  .n_untypeds = %lu\n", bi->n_untypeds);

  for (word_t i = 0; i < bi->n_untypeds; i++)
    {
      printf ("  .untyped[%lu]: paddr = %016lx, size = %lu\n", i,
              bi->untypeds[i].base, 1ul << bi->untypeds[i].size_bits);
    }

  int untyped = init_cap_first_untyped;

  untyped_retype (untyped, cap_tcb, 0, init_cap_root_cnode, init_cap_root_cnode, 64,
                  100, 1);
  untyped_retype (untyped, cap_endpoint, 0, init_cap_root_cnode, init_cap_root_cnode,
                  64, 101, 1);
  // cnode_debug_print (init_cap_root_cnode);

  tcb_configure (100, 0, init_cap_root_cnode, 0, init_cap_init_vspace, 0,
                 (word_t)ipc_buffer + 1024, 0);
  tcb_set_tls_base (100, (uintptr_t)&tls2.self);

  frame_t frame;
  tcb_read_registers (init_cap_init_tcb, false, 0, 0, &frame);

  frame.rip = (uintptr_t)thread_entry;
  frame.rsp = (uintptr_t)thread_stack + sizeof (thread_stack);
  frame.rdi = (uintptr_t)ipc_buffer + 1024;
  frame.rsi = 42;

  tcb_write_registers (100, false, 0, 0, &frame);

  printf ("Starting new userland thread\n");
  tcb_resume (100);
  yield ();

  while (true)
    {
      word_t badge;
      word_t msg = 1;
      message_info_t resp = recv (101, &badge);
      if (get_message_length (resp) > 0)
        printf ("received %lu\n", (msg = get_mr (0)));
      if (msg % 2 == 0)
        yield ();

      if (get_message_label (resp) == 1)
        break;
    }

  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
