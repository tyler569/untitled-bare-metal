#include "pci.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "tar.h"

#include "./lib.h"
#include "./pci_util.h"

#include "./calculator_server.h"

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

struct untyped_info
{
  short size_bits;
  short index;
  bool in_use;
};

int
compare_untyped_info (const void *a, const void *b)
{
  const struct untyped_info *ua = a;
  const struct untyped_info *ub = b;

  return ub->size_bits - ua->size_bits;
}

struct untyped_info untypeds[256];

void
setup_memory_information (struct boot_info *bi)
{
  for (size_t i = 0; i < bi->n_untypeds; i++)
    {
      untypeds[i].size_bits = bi->untypeds[i].size_bits;
      untypeds[i].index = i;
    }

  qsort (untypeds, bi->n_untypeds, sizeof (struct untyped_info),
         compare_untyped_info);
}

void
map_huge_page ()
{
  word_t untyped = init_cap_first_untyped + untypeds[1].index;

  cptr_t page = allocate (untyped, cap_x86_64_huge_page, 1);

  printf ("Huge page: %lx\n", page);

  int err = x86_64_huge_page_map (page, init_cap_init_vspace, 0x60'0000, 0xff);
  if (err != no_error)
    {
    printf ("Failed to map huge page: %s\n", error_string (err));
    return;
    }
  printf ("Mapped huge page\n");

  for (size_t i = 0; i < 0x20'0000; i++)
    {
      volatile uint8_t *ptr = (volatile uint8_t *)(0x60'0000 + i);
      *ptr = 0x42;
    }

  err = x86_64_huge_page_map (page, init_cap_init_vspace, 0x120'0000, 0xff);
  if (err != no_error)
    {
    printf ("Failed to map huge page: %s\n", error_string (err));
    return;
    }
  printf ("Mapped huge page\n");

  for (size_t i = 0; i < 0x20'0000; i++)
    {
      volatile uint8_t *ptr = (volatile uint8_t *)(0x120'0000 + i);
      if (*ptr != 0x42)
        {
      printf ("Failed to read back value\n");
      return;
        }
    }
}

[[noreturn]] int
c_start (void *ipc_buffer, void *boot_info)
{
  __ipc_buffer = ipc_buffer;

  bi = boot_info;
  cptr_alloc_init (bi);

  print_bootinfo_information ();
  print_to_e9 ("Hello World!\n");

  setup_memory_information (bi);

  word_t untyped = init_cap_first_untyped + untypeds[0].index;

  printf ("Largest untyped: %lx\n", untyped);

  map_huge_page ();

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

  for (const char *c = "Hello Serial"; *c; c++)
    {
      set_mr (0, *c);
      message_info_t info = new_message_info (1, 0, 0, 1);
      send (serial_endpoint, info);
    }

  word_t badge;
  word_t a = 0, b = 1;

  while (true)
    {
      message_info_t info;
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
