#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "tar.h"

#include "./lib.h"

#define PAGE_SIZE 4096

extern inline long write (FILE *, const void *str, unsigned long len);

uint8_t __attribute__ ((aligned (PAGE_SIZE))) stack[PAGE_SIZE * 4];
extern char __executable_start;
struct ipc_buffer *__ipc_buffer;
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

[[noreturn]] int
c_start (void *ipc_buffer, void *boot_info)
{
  __ipc_buffer = ipc_buffer;

  bi = boot_info;
  cptr_alloc_init (bi);

  print_bootinfo_information ();
  print_to_e9 ("Hello World!");

  cptr_t untyped = init_cap_first_untyped;

  cptr_t endpoint_cap = allocate (untyped, cap_endpoint, 1);

  void *proctest_elf = find_tar_entry (bi->initrd, "testproc");

  if (!proctest_elf)
    printf ("Failed to find testproc in initrd\n");
  else
    {
      cptr_t proc_tcb_cap;
      int err = create_process (proctest_elf, 0, untyped, init_cap_init_vspace,
                                &proc_tcb_cap, nullptr);

      frame_t frame;
      tcb_read_registers (proc_tcb_cap, false, 0, 0, &frame);
      frame.rsi = endpoint_cap;
      tcb_write_registers (proc_tcb_cap, false, 0, 0, &frame);

      if (err)
        printf ("Error creating process: %d\n", err);
      else
        printf ("Successfully created process\n");

      tcb_resume (proc_tcb_cap);
    }

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

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
