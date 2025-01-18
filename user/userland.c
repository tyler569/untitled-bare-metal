#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#define PAGE_SIZE 4096

#include "lib.c"

struct thread_local_storage
{
  char data[512 - 8];
  void *self;
};

struct thread_local_storage tls1 = { {}, (void *)&tls1.self };
struct thread_local_storage tls2 = { {}, (void *)&tls2.self };

extern char __executable_start;
thread_local struct ipc_buffer *__ipc_buffer;
struct boot_info *bi = nullptr;
int next_unused_cap = -1;
uint8_t __attribute__ ((aligned (PAGE_SIZE))) thread_stack[PAGE_SIZE * 4];
uint8_t __attribute__ ((aligned (PAGE_SIZE))) stack[PAGE_SIZE * 4];


cptr_t
get_port_cap ()
{
  static cptr_t port_cap = 0;

  if (port_cap != 0)
    {
      return port_cap;
    }

  port_cap = next_unused_cap++;

  x86_64_io_port_control_issue (init_cap_io_port_control, 0x0, 0xffff,
                                init_cap_root_cnode, port_cap, 64);

  return port_cap;
}

void
print_to_e9 (const char *string)
{
  cptr_t port_cap = get_port_cap ();

  for (const char *c = string; *c; c++)
    {
      x86_64_io_port_out8 (port_cap, 0xe9, *c);
    }
}

void
print_bootinfo_information()
{
  printf ("Hello, World from userland; ipc buffer %p!\n", __ipc_buffer);
  printf ("executable_start: %p\n", &__executable_start);

  printf ("Boot info: %p\n", bi);
  printf ("  .untyped_range = [%zu, %zu)\n", bi->untyped_range.start,
          bi->untyped_range.end);
  printf ("  .empty_range = [%zu, %zu)\n", bi->empty_range.start,
          bi->empty_range.end);
  printf ("  .n_untypeds = %lu\n", bi->n_untypeds);

  for (word_t i = 0; i < bi->n_untypeds; i++)
    {
      printf ("  .untyped[%lu]: paddr = %016lx, size = %lu\n", i,
              bi->untypeds[i].base, 1ul << bi->untypeds[i].size_bits);
    }
}

cptr_t
create_thread (void *ipc_buffer, void *entry, cptr_t *endpoint)
{
  int untyped = init_cap_first_untyped;
  int tcb_cap = next_unused_cap++;
  int endpoint_cap = next_unused_cap++;
  *endpoint = endpoint_cap;

  untyped_retype (untyped, cap_tcb, 0, init_cap_root_cnode,
                  init_cap_root_cnode, 64, tcb_cap, 1);
  untyped_retype (untyped, cap_endpoint, 0, init_cap_root_cnode,
                  init_cap_root_cnode, 64, endpoint_cap, 1);

  tcb_configure (tcb_cap, 0, init_cap_root_cnode, 0, init_cap_init_vspace, 0,
                 (word_t)ipc_buffer + 1024, 0);
  tcb_set_tls_base (tcb_cap, (uintptr_t)&tls2.self);

  frame_t frame;
  tcb_read_registers (init_cap_init_tcb, false, 0, 0, &frame);

  frame.rip = (uintptr_t)entry;
  frame.rsp = (uintptr_t)thread_stack + sizeof (thread_stack);
  frame.rdi = (uintptr_t)ipc_buffer + 1024;
  frame.rsi = endpoint_cap;

  tcb_write_registers (tcb_cap, false, 0, 0, &frame);

  return tcb_cap;
}

[[noreturn]] int
c_start (void *ipc_buffer, void *boot_info)
{
  asm volatile ("wrfsbase %0" ::"r"(&tls1.self));
  __ipc_buffer = ipc_buffer;

  tcb_set_tls_base (init_cap_init_tcb, (uintptr_t)&tls1.self);

  bi = boot_info;
  next_unused_cap = bi->empty_range.start;

  print_to_e9 ("[E9] Hello World!\n");

  print_bootinfo_information ();

  cptr_t tcb_cap;
  cptr_t endpoint_cap;

  [[noreturn]] void thread_entry (void *ipc_buffer, uintptr_t arg);
  tcb_cap = create_thread(ipc_buffer + 1024, thread_entry, &endpoint_cap);
  tcb_resume (tcb_cap);

  yield ();

  while (true)
    {
      word_t badge;
      word_t msg = 1;
      message_info_t resp = recv (endpoint_cap, &badge);
      if (get_message_length (resp) > 0)
        printf ("received %lu\n", (msg = get_mr (0)));
      if (msg % 2 == 0)
        yield ();

      if (get_message_label (resp) == 1)
        break;
    }

  exit ();
}

[[noreturn]] void
thread_entry (void *ipc_buffer, uintptr_t endpoint_cap)
{
  asm volatile ("wrfsbase %0" ::"r"(&tls2.self));
  __ipc_buffer = ipc_buffer;
  printf ("Hello, World from userland thread! Arg is %lu\n", endpoint_cap);

  message_info_t info = new_message_info (0, 0, 0, 1);
  for (word_t i = 0; i < 10; i++)
    {
      set_mr (0, i);
      send (endpoint_cap, info);
    }

  info = new_message_info (1, 0, 0, 0);
  send (endpoint_cap, info);

  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
