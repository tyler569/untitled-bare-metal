#include "limine.h"
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
#include "./serial_driver.h"

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

  printf ("  limine bootloader: %p\n", bi->framebuffer_info.address);
  printf ("    size: %zx\n", bi->framebuffer_info.height
                                 * bi->framebuffer_info.width
                                 * bi->framebuffer_info.bpp / 8);
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
spawn_calculator_thread (cptr_t untyped, cptr_t calculator_endpoint)
{
  void *calculator_elf = find_tar_entry (bi->initrd, "calculator_server");
  if (!calculator_elf)
    {
      printf ("Could not find calculator_elf\n");
      return;
    }

  struct thread_data td = {
    .elf_header = calculator_elf,
    .untyped = untyped,
    .scratch_vspace = init_cap_init_vspace,
    .name = "calculator_server",
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

void
spawn_serial_driver (cptr_t untyped, cptr_t serial_write_endpoint,
                     cptr_t serial_read_endpoint)
{
  void *serial_driver_elf = find_tar_entry (bi->initrd, "serial_driver");
  void *serial_broker_elf = find_tar_entry (bi->initrd, "serial_broker");

  if (!serial_driver_elf || !serial_broker_elf)
    {
      printf ("Could not find serial driver elf\n");
      return;
    }

  int err;

  cptr_t cnode = allocate_with_size (untyped, cap_cnode, 1, 4);

  // serial_cnode_cap
  cnode_copy (cnode, serial_cnode_cap, 64, init_cap_root_cnode, cnode, 64,
              cap_rights_all);

  // serial_serial_port_cap
  x86_64_io_port_control_issue (init_cap_io_port_control, 0x3f8, 0x3ff, cnode,
                                serial_serial_port_cap, 64);

  // serial_endpoint_cap
  cnode_copy (cnode, serial_endpoint_cap, 64, init_cap_root_cnode,
              serial_write_endpoint, 64, cap_rights_all);

  // serial_irq_cap
  irq_control_get (init_cap_irq_control, 4, cnode, serial_irq_cap, 64);

  // serial_notification_cap
  untyped_retype (untyped, cap_notification, 0, init_cap_root_cnode, cnode, 64,
                  serial_notification_cap, 1);

  // serial_badged_notification_cap
  err = cnode_mint (cnode, serial_irq_notification_cap, 64, cnode,
                    serial_notification_cap, 64, cap_rights_all, 0xFFFF);
  if (err)
    printf ("Failed to mint badged notification cap 1: %d\n", err);

  // serial_broker_notification_cap
  cptr_t sb_notification_cap = allocate (untyped, cap_notification, 1);
  err = cnode_mint (cnode, serial_broker_notification_cap, 64,
                    init_cap_root_cnode, sb_notification_cap, 64,
                    cap_rights_all, 1);
  if (err)
    printf ("Failed to mint badged notification cap 2: %d\n", err);

  // serial_broker_endpoint_cap
  cnode_copy (cnode, serial_broker_endpoint_cap, 64, init_cap_root_cnode,
              serial_read_endpoint, 64, cap_rights_all);

  cptr_t ring_page = allocate (untyped, cap_x86_64_page, 1);

  struct thread_data tdb = {
    .elf_header = serial_broker_elf,
    .untyped = untyped,
    .scratch_vspace = init_cap_init_vspace,
    .cspace_root = cnode,
    .name = "serial_broker",
  };

  struct thread_data tdd = {
    .elf_header = serial_driver_elf,
    .untyped = untyped,
    .scratch_vspace = init_cap_init_vspace,
    .cspace_root = cnode,
    .name = "serial_driver",
  };

  err = spawn_thread (&tdb);

  if (err)
    printf ("Error creating serial_broker process\n");
  else
    printf ("Successfully created serial_broker process\n");

  err = spawn_thread (&tdd);

  if (err)
    printf ("Error creating serial_driver process\n");
  else
    printf ("Successfully created serial_driver process\n");

  map_page (untyped, tdb.vspace, ring_page, 0x120'0000);
  map_page (untyped, tdd.vspace, ring_page, 0x120'0000);

  // serial_tcb_cap
  cnode_copy (cnode, serial_tcb_cap, 64, init_cap_root_cnode, tdd.tcb, 64,
              cap_rights_all);

  // tcb_set_debug (td.tcb, true);
  tcb_resume (tdb.tcb);
  tcb_resume (tdd.tcb);
}

void
calculate_fibonacci_numbers (cptr_t calculator_endpoint, word_t up_to)
{
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

      if (a > up_to)
        break;
    }
}

void
print_to_serial (cptr_t serial_write_endpoint, const char *message)
{
  for (const char *c = message; *c; c++)
    {
      set_mr (0, *c);
      message_info_t info = new_message_info (1, 0, 0, 1);
      send (serial_write_endpoint, info);
    }
}

void
serial_capitalization_server (cptr_t serial_write_endpoint,
                              cptr_t serial_read_endpoint)
{
  while (true)
    {
      word_t badge;
      message_info_t info = new_message_info (serial_driver_read, 0, 0, 0);
      info = call (serial_read_endpoint, info, &badge);
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

      info = new_message_info (serial_driver_write, 0, 0, regs);
      send (serial_write_endpoint, info);
    }
}

void *
map_framebuffer (cptr_t untyped, size_t *fbsize)
{
  struct limine_framebuffer fbinfo = bi->framebuffer_info;
  *fbsize = fbinfo.height * fbinfo.width * fbinfo.bpp / 8;
  uintptr_t fbaddr
      = (uintptr_t)fbinfo.address - 0xffff800000000000;

  cptr_t fbuntyped = 0;

  for (size_t i = 0; i < bi->n_untypeds; i++)
    {
      if (!bi->untypeds[i].is_device)
        continue;
      if (bi->untypeds[i].base == fbaddr)
        {
          fbuntyped = init_cap_first_untyped + i;
          break;
        }
    }

  if (fbuntyped == 0)
    {
      printf ("Failed to find untyped for framebuffer\n");
      return nullptr;
    }

  for (size_t page_offset = 0; page_offset < *fbsize; page_offset += 0x1000)
    {
      uintptr_t addr = fbaddr + page_offset;
      cptr_t cptr = cptr_alloc ();
      int err = untyped_retype (fbuntyped, cap_x86_64_page, 21,
                                init_cap_root_cnode, init_cap_root_cnode, 64,
                                cptr, 1);
      if (err != no_error)
        {
          printf ("Error: could not allocate framebuffer page: (%s)\n",
                  error_string (err));
          return nullptr;
        }

      err = map_page (untyped, init_cap_init_vspace, cptr, addr);
      if (err != no_error)
        {
          printf ("Error: could not map framebuffer page: (%s)\n",
                  error_string (err));
          return nullptr;
        }
    }

  return (void *)fbaddr;
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

  cptr_t calculator_endpoint = allocate (untyped, cap_endpoint, 1);
  cptr_t serial_write_endpoint = allocate (untyped, cap_endpoint, 1);
  cptr_t serial_read_endpoint = allocate (untyped, cap_endpoint, 1);

  pci_io_port = cptr_alloc ();

  x86_64_io_port_control_issue (init_cap_io_port_control, 0xcf8, 0xcff,
                                init_cap_root_cnode, pci_io_port, 64);

  enumerate_pci_bus (pci_io_port);

  size_t fbsize;
  void *fb = map_framebuffer (untyped, &fbsize);

  for (size_t i = 0; i < fbsize; i++)
    ((char *)fb)[i] = 0xff;

  spawn_calculator_thread (untyped, calculator_endpoint);
  spawn_serial_driver (untyped, serial_write_endpoint, serial_read_endpoint);

  print_to_serial (serial_write_endpoint, "Hello, Serial World!\n");

  calculate_fibonacci_numbers (calculator_endpoint, 100'000);
  serial_capitalization_server(serial_write_endpoint, serial_read_endpoint);

  exit (0);
}

[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm volatile ("movq %0, %%rsp" ::"r"(stack + sizeof (stack)));
  asm volatile ("jmp c_start");
}
