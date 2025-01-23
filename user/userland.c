#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"

#include "./lib.h"

#define PAGE_SIZE 4096

extern inline long write (FILE *, const void *str, unsigned long len);

[[noreturn]] void thread_entry (void *ipc_buffer, uintptr_t arg);

struct thread_local_storage
{
  char data[512 - 8];
  void *self;
};

struct thread_local_storage tls1 = { {}, (void *)&tls1.self };
struct thread_local_storage tls2 = { {}, (void *)&tls2.self };
uint8_t __attribute__ ((aligned (PAGE_SIZE))) stack[PAGE_SIZE * 4];
uint8_t __attribute__ ((aligned (PAGE_SIZE))) thread_stack[PAGE_SIZE * 4];

extern char __executable_start;

thread_local struct ipc_buffer *__ipc_buffer;
struct boot_info *bi = nullptr;

cptr_t
get_port_cap ()
{
  static cptr_t port_cap = 0;

  if (port_cap != 0)
    return port_cap;

  port_cap = cptr_alloc ();

  x86_64_io_port_control_issue (init_cap_io_port_control, 0x0, 0xffff,
                                init_cap_root_cnode, port_cap, 64);

  return port_cap;
}

void
print_to_e9 (const char *string)
{
  cptr_t port_cap = get_port_cap ();

  for (const char *c = string; *c; c++)
    x86_64_io_port_out8 (port_cap, 0xe9, *c);
}

void
print_bootinfo_information ()
{
  printf ("\n");
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
  printf ("\n");
}

cptr_t
create_thread (void *ipc_buffer, void *entry, cptr_t *endpoint)
{
  int untyped = init_cap_first_untyped;
  int tcb_cap = cptr_alloc ();
  int endpoint_cap = cptr_alloc ();
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
  cptr_alloc_init (bi);

  print_bootinfo_information ();

  print_to_e9 ("Hello World!");

  // create_process (bi->init_elf, 0, 0, nullptr, nullptr);

  cptr_t untyped = init_cap_first_untyped;

  cptr_t buffer_base = create_buffer (untyped, 4);
  map_buffer (untyped, init_cap_init_vspace, buffer_base, 4, 0x700000);
  unsigned char *buffer = (unsigned char *)0x700000;

  for (size_t i = 0; i < 4 * 4096; i++)
    buffer[i] = 0xaa;
  for (size_t i = 0; i < 4 * 4096; i++)
    if (buffer[i] != 0xaa)
      printf ("Buffer not zeroed\n");

  cptr_t tcb_cap;
  cptr_t endpoint_cap;

  tcb_cap = create_thread (ipc_buffer + 1024, thread_entry, &endpoint_cap);
  tcb_resume (tcb_cap);

  yield ();

  word_t badge;

  set_mr (0, 30);
  call (endpoint_cap, new_message_info (1, 0, 0, 1), &badge);
  printf ("Method 1, Response: %lu\n", get_mr (0));

  call (endpoint_cap, new_message_info (2, 0, 0, 1), &badge);
  printf ("Method 2, Response: %lu\n", get_mr (0));

  call (endpoint_cap, new_message_info (3, 0, 0, 1), &badge);
  printf ("Method 3, Response: %lu\n", get_mr (0));

  exit ();
}

[[noreturn]] void
thread_entry (void *ipc_buffer, uintptr_t endpoint_cap)
{
  __ipc_buffer = ipc_buffer;
  printf ("Hello, World from userland thread! Arg is %lu\n", endpoint_cap);

  bool done = false;
  message_info_t info, resp;
  word_t badge;

  info = recv (endpoint_cap, &badge);

  while (!done)
    {
      word_t label = get_message_label (info);

      switch (label)
        {
        case 0:
          done = true;
          break;
        case 1:
          set_mr (0, 42);
          break;
        case 2:
          set_mr (0, get_mr (0) * 2);
          break;
        default:
          set_mr (0, get_message_label (info));
          break;
        }

      resp = new_message_info (label, 0, 0, 1);
      info = reply_recv (endpoint_cap, resp, &badge);
    }

  exit ();
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
