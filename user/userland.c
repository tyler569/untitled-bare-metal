#include "pci.h"
#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "tar.h"

#include "./lib.h"
#include "./pci_util.h"

#include "./calculator_server.h"
#include "./captest.h"

#define PAGE_SIZE 4096

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

  size_t total_untyped_size = 0;

  for (word_t i = 0; i < bi->n_untypeds; i++)
    {
      printf ("  .untyped[%lu]: paddr = %016lx, size = %lu, is_device = %i\n",
              i, bi->untypeds[i].base, 1ul << bi->untypeds[i].size_bits,
              bi->untypeds[i].is_device);

      if (!bi->untypeds[i].is_device)
        total_untyped_size += 1ul << bi->untypeds[i].size_bits;
    }
  printf ("Total untyped size: %zu (%zu MB)\n", total_untyped_size,
          total_untyped_size / 1024 / 1024);
  printf ("\n");
}

cptr_t pci_io_port;

[[noreturn]] void
halt_forever ()
{
  printf ("Halting forever\n");
  tcb_suspend (init_cap_init_tcb);
  unreachable ();
}

[[noreturn]] int
c_start (void *ipc_buffer, void *boot_info)
{
  __ipc_buffer = ipc_buffer;

  bi = boot_info;
  cptr_alloc_init (bi);

  print_bootinfo_information ();
  print_to_e9 ("Hello World!\n");

  word_t untyped = 0;
  word_t largest_untyped_size = 0;
  for (size_t i = 0; i < bi->n_untypeds; i++)
    if (bi->untypeds[i].size_bits > largest_untyped_size)
      {
        untyped = init_cap_first_untyped + i;
        largest_untyped_size = bi->untypeds[i].size_bits;
      }

  printf ("Largest untyped: %lx (size: %lu)\n", untyped, largest_untyped_size);

  cptr_t calculator_endpoint = allocate (untyped, cap_endpoint, 1);
  cptr_t serial_endpoint = allocate (untyped, cap_endpoint, 1);

  cptr_t serial_data_available = allocate (untyped, cap_notification, 1);
  cptr_t badged_serial_data_available = cptr_alloc ();

  cnode_mint (init_cap_root_cnode, badged_serial_data_available, 64,
              init_cap_root_cnode, serial_data_available, 64, cap_rights_all,
              1);

  pci_io_port = cptr_alloc ();

  x86_64_io_port_control_issue (init_cap_io_port_control, 0xcf8, 0xcff,
                                init_cap_root_cnode, pci_io_port, 64);

  cptr_t cnode = allocate_with_size (untyped, cap_cnode, 6, 1);
  printf ("CNode: %lx\n", cnode);

  enumerate_pci_bus (pci_io_port);

  void *calculator_elf = find_tar_entry (bi->initrd, "calculator_server");
  void *serial_driver_elf = find_tar_entry (bi->initrd, "serial_driver");
  void *captest_elf = find_tar_entry (bi->initrd, "captest");

  if (!calculator_elf)
    {
      printf ("Failed to find testproc in initrd\n");
      halt_forever ();
    }

  if (!serial_driver_elf)
    {
      printf ("Failed to find serial_driver in initrd\n");
      halt_forever ();
    }

  if (!captest_elf)
    {
      printf ("Failed to find captest in initrd\n");
      halt_forever ();
    }

  {
    struct thread_data td = {
      .elf_header = calculator_elf,
      .untyped = untyped,
      .scratch_vspace = init_cap_init_vspace,
      .arguments[0] = calculator_endpoint,
    };

    int err = spawn_thread (&td);
    if (err)
      printf ("Error spawning thread: %d\n", err);
    else
      {
        printf ("Successfully spawned thread\n");
        tcb_resume (td.tcb);
      }
  }

  {
    cptr_t serial_port_cap = cptr_alloc ();
    x86_64_io_port_control_issue (init_cap_io_port_control, 0x3f8, 0x3ff,
                                  init_cap_root_cnode, serial_port_cap, 64);

    const cptr_t irq_cap = cptr_alloc ();
    const cptr_t irq_nfn_cap = allocate (untyped, cap_notification, 1);
    const cptr_t badged_irq_nfn_cap = cptr_alloc ();

    irq_control_get (init_cap_irq_control, 4, init_cap_root_cnode, irq_cap,
                     64);
    cnode_mint (init_cap_root_cnode, badged_irq_nfn_cap, 64,
                init_cap_root_cnode, irq_nfn_cap, 64, cap_rights_all, 0xFFFF);
    irq_handler_set_notification (irq_cap, badged_irq_nfn_cap);

    struct thread_data td = {
      .elf_header = serial_driver_elf,
      .untyped = untyped,
      .scratch_vspace = init_cap_init_vspace,
      .arguments[0] = serial_port_cap,
      .arguments[1] = serial_endpoint,
      .arguments[2] = irq_cap,
      .arguments[3] = badged_serial_data_available,
    };
    int err = spawn_thread (&td);

    if (err)
      printf ("Error creating serial_driver process\n");
    else
      printf ("Successfully created serial_driver process\n");

    tcb_bind_notification (td.tcb, irq_nfn_cap);
    tcb_resume (td.tcb);
  }

  {
    cptr_t ep_cap = allocate (untyped, cap_endpoint, 1);
    cptr_t cnode_cap = allocate_with_size (untyped, cap_cnode, 1, 4);

    cnode_copy (cnode_cap, captest_endpoint, 64, init_cap_root_cnode, ep_cap,
                64, cap_rights_all);

    struct thread_data td = {
      .elf_header = captest_elf,
      .untyped = untyped,
      .scratch_vspace = init_cap_init_vspace,
      .cspace_root = cnode_cap,
    };

    int err = spawn_thread (&td);

    if (err)
      printf ("Error creating captest process\n");
    else
      {
        printf ("Successfully created captest process\n");
        tcb_resume (td.tcb);
      }

    for (int i = 0; i < 5; i++)
      {
        printf ("Calling captest\n");
        word_t reply_badge;
        call (ep_cap, 0, &reply_badge);
      }
  }

  // yield ();

  for (const char *c = "Hello Serial"; *c; c++)
    {
      set_mr (0, *c);
      message_info_t info = new_message_info (1, 0, 0, 1);
      send (serial_endpoint, info);
    }

  word_t badge;

  // word_t number = 1;
  word_t a = 0, b = 1;

  while (true)
    {
      message_info_t info;
      // set_mr (0, number);
      // info = new_message_info (calculator_ret42, 0, 0, 1);
      // call (calculator_endpoint, info, &badge);
      // printf ("Method 1, Response: %lu\n", (number = get_mr (0)));

      // set_mr (0, number);
      // info = new_message_info (calculator_double, 0, 0, 1);
      // call (calculator_endpoint, info, &badge);
      // printf ("Method 2, Response: %lu\n", (number = get_mr (0)));

      set_mr (0, a);
      set_mr (1, b);
      info = new_message_info (calculator_add, 0, 0, 2);
      call (calculator_endpoint, info, &badge);
      a = b;
      b = get_mr (0);

      printf ("Fibonacci: %lu\n", a);

      if (a > 100000)
        break;
    }

  while (true)
    {
      word_t badge;
      message_info_t info = new_message_info (2, 0, 0, 0);

      wait (serial_data_available, &badge);

      info = call (serial_endpoint, info, &badge);
      size_t regs = get_message_length (info);

      if (regs == 0)
        continue;

      printf ("Serial: ");

      for (size_t i = 0; i < regs; i++)
        {
          uint8_t byte = get_mr (i);
          printf ("%c", byte);

          if (byte >= 'a' && byte <= 'z')
            set_mr (i, byte + 'A' - 'a');
        }

      send (serial_endpoint, new_message_info (1, 0, 0, regs));
    }

  exit (0);
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
