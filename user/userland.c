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
  print_to_e9 ("Hello World!\n");

  cptr_t untyped = init_cap_first_untyped;

  cptr_t endpoint_cap = allocate (untyped, cap_endpoint, 1);
  cptr_t notification_cap = allocate (untyped, cap_notification, 1);
  cptr_t badged_notification_cap = cptr_alloc ();

  cnode_mint (init_cap_root_cnode, badged_notification_cap, 64,
              init_cap_root_cnode, notification_cap, 64, cap_rights_all, 1);

  cptr_t serial_endpoint = allocate (untyped, cap_endpoint, 1);

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

  void *serial_driver_elf = find_tar_entry (bi->initrd, "serial_driver");

  if (!serial_driver_elf)
    printf ("Failed to find serial_driver in initrd\n");
  else
    {
      cptr_t proc_serial_driver_cap;
      int err = create_process (serial_driver_elf, 0, untyped,
                                init_cap_init_vspace, &proc_serial_driver_cap,
                                nullptr);

      if (err)
        printf ("Error creating serial_driver process: %d\n", err);
      else
        printf ("Successfully created serial_driver process\n");

      cptr_t serial_port_cap = cptr_alloc ();
      x86_64_io_port_control_issue (init_cap_io_port_control, 0x3f8, 0x3ff,
                                    init_cap_root_cnode, serial_port_cap, 64);

      const cptr_t irq_cap = cptr_alloc ();
      const cptr_t irq_nfn_cap = allocate (untyped, cap_notification, 1);
      const cptr_t badged_irq_nfn_cap = cptr_alloc ();

      irq_control_get (init_cap_irq_control, 4, init_cap_root_cnode, irq_cap,
                       64);
      cnode_mint (init_cap_root_cnode, badged_irq_nfn_cap, 64,
                  init_cap_root_cnode, irq_nfn_cap, 64, cap_rights_all, 0xFFFF'FFFF);
      tcb_bind_notification (proc_serial_driver_cap, irq_nfn_cap);
      irq_handler_set_notification (irq_cap, badged_irq_nfn_cap);

      frame_t frame;
      tcb_read_registers (proc_serial_driver_cap, false, 0, 0, &frame);
      frame.rsi = serial_port_cap;
      frame.rdx = serial_endpoint;
      frame.rcx = irq_cap;
      tcb_write_registers (proc_serial_driver_cap, false, 0, 0, &frame);

      tcb_resume (proc_serial_driver_cap);
    }

  // yield ();

  for (const char *c = "Hello Serial"; *c; c++)
    {
      set_mr (0, *c);
      message_info_t info = new_message_info (1, 0, 0, 1);
      send (serial_endpoint, info);
    }

  word_t notification_word = 0;

  tcb_bind_notification (init_cap_init_tcb, notification_cap);

  printf ("waiting for notification (but it's bound so not really)\n");
  // wait (notification_cap, &notification_word);
  recv (endpoint_cap, &notification_word);
  printf ("got notification; word is %lu\n", notification_word);

  word_t badge;

  // word_t number = 1;
  word_t a = 0, b = 1;

  while (true)
    {
      // set_mr (0, number);
      // call (endpoint_cap, new_message_info (1, 0, 0, 1), &badge);
      // printf ("Method 1, Response: %lu\n", (number = get_mr (0)));

      // set_mr (0, number);
      // call (endpoint_cap, new_message_info (2, 0, 0, 1), &badge);
      // printf ("Method 2, Response: %lu\n", (number = get_mr (0)));

      set_mr (0, a);
      set_mr (1, b);
      call (endpoint_cap, new_message_info (4, 0, 0, 2), &badge);
      a = b;
      b = get_mr (0);

      printf ("Fibonacci: %lu\n", a);

      if (a > 100000)
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
