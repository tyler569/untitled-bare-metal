#include "limine.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/bootinfo.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "tar.h"

#include "./lib.h"
#include "./pci_util.h"

#include "./calculator_server.h"
#include "./serial_driver.h"
#include <stddef.h>

[[maybe_unused]] constexpr size_t PAGE_SIZE = 4096;

struct boot_info *bi = nullptr;

void
print_to_e9 (cptr_t port_cap, const char *string)
{
  for (const char *c = string; *c; c++)
    x86_64_io_port_out8 (port_cap, 0xE9, *c);
}

void
print_bootinfo_information ()
{
  printf ("\n");
  printf ("Hello, World from userland; ipc buffer %p!\n", __ipc_buffer);

  printf ("Boot info: %p\n", bi);
  printf ("  .untyped_range = [%zu, %zu)\n", bi->untyped_range.start,
          bi->untyped_range.end);
  printf ("  .empty_range = [%zu, %zu)\n", bi->empty_range.start,
          bi->empty_range.end);
  printf ("  .n_untypeds = %lu\n", bi->n_untypeds);

  size_t total_untyped_size = 0;

  for (word_t i = 0; i < bi->n_untypeds; i++)
    {
      printf ("  .untyped[%lu]: paddr = %016lX, size = %lu, is_device = %i\n",
              i, bi->untypeds[i].base, 1UL << bi->untypeds[i].size_bits,
              bi->untypeds[i].is_device);

      if (!bi->untypeds[i].is_device)
        total_untyped_size += 1UL << bi->untypeds[i].size_bits;
    }
  printf ("Total untyped size: %zu (%zu MB)\n", total_untyped_size,
          total_untyped_size / 1024 / 1024);
  printf ("\n");

  printf ("  limine fb: %p, size: %zX\n", bi->framebuffer_info.address,
          bi->framebuffer_info.height * bi->framebuffer_info.width
              * bi->framebuffer_info.bpp / 8);
}

[[noreturn]] void
halt_forever ()
{
  printf ("Halting forever\n");
  tcb_suspend (INIT_CAP_INIT_TCB);
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
  const void *calculator_elf
      = find_tar_entry (bi->initrd, "calculator_server");
  if (!calculator_elf)
    {
      printf ("Could not find calculator_elf\n");
      return;
    }

  struct thread_data td = {
    .elf_header = calculator_elf,
    .untyped = untyped,
    .scratch_vspace = INIT_CAP_INIT_VSPACE,
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
spawn_serial_driver (cptr_t untyped, cptr_t serial_endpoint,
                     cptr_t serial_read_ntfn)
{
  const void *serial_driver_elf = find_tar_entry (bi->initrd, "serial_driver");

  if (!serial_driver_elf)
    {
      printf ("Could not find serial driver elf\n");
      return;
    }

  int err = 0;

  const cptr_t cnode = allocate_with_size (untyped, CAP_CNODE, 1, 4);

  // serial_cnode_cap
  cnode_copy (cnode, SERIAL_CNODE_CAP, 64, INIT_CAP_ROOT_CNODE, cnode, 64,
              CAP_RIGHTS_ALL);

  // serial_serial_port_cap
  x86_64_io_port_control_issue (INIT_CAP_IO_PORT_CONTROL, 0x3F8, 0x3FF, cnode,
                                SERIAL_SERIAL_PORT_CAP, 64);

  // serial_endpoint_cap
  cnode_copy (cnode, SERIAL_ENDPOINT_CAP, 64, INIT_CAP_ROOT_CNODE,
              serial_endpoint, 64, CAP_RIGHTS_ALL);

  // serial_read_notification_cap
  cnode_copy (cnode, SERIAL_READ_NOTIFICATION_CAP, 64, INIT_CAP_ROOT_CNODE,
              serial_read_ntfn, 64, CAP_RIGHTS_ALL);

  // serial_irq_cap
  irq_control_get (INIT_CAP_IRQ_CONTROL, 4, cnode, SERIAL_IRQ_CAP, 64);

  // serial_notification_cap
  untyped_retype (untyped, CAP_NOTIFICATION, 0, INIT_CAP_ROOT_CNODE, cnode, 64,
                  SERIAL_NOTIFICATION_CAP, 1);

  // serial_badged_notification_cap
  err = cnode_mint (cnode, SERIAL_IRQ_NOTIFICATION_CAP, 64, cnode,
                    SERIAL_NOTIFICATION_CAP, 64, CAP_RIGHTS_ALL, 0xFFFF);
  if (err)
    printf ("Failed to mint badged notification cap 1: %d\n", err);

  struct thread_data tdd = {
    .elf_header = serial_driver_elf,
    .untyped = untyped,
    .scratch_vspace = INIT_CAP_INIT_VSPACE,
    .cspace_root = cnode,
    .name = "serial_driver",
  };

  err = spawn_thread (&tdd);

  if (err)
    printf ("Error creating serial_driver process\n");
  else
    printf ("Successfully created serial_driver process\n");

  // serial_tcb_cap
  cnode_copy (cnode, SERIAL_TCB_CAP, 64, INIT_CAP_ROOT_CNODE, tdd.tcb, 64,
              CAP_RIGHTS_ALL);

  tcb_resume (tdd.tcb);
}

void
spawn_cdt_test (cptr_t main_untyped, cptr_t test_untyped)
{
  const void *cdt_test_elf = find_tar_entry (bi->initrd, "cdt_test");
  if (!cdt_test_elf)
    {
      printf ("Could not find cdt_test elf\n");
      return;
    }

  // Create a cnode for the CDT test using the main untyped
  const cptr_t test_cnode = allocate_with_size (main_untyped, CAP_CNODE, 1, 7);

  // Copy the cnode cap into the test cnode itself (slot 0)
  cnode_copy (test_cnode, 0, 64, INIT_CAP_ROOT_CNODE, test_cnode, 64,
              CAP_RIGHTS_ALL);

  // Delegate the TEST untyped into slot 1 (so test can revoke it safely)
  cnode_copy (test_cnode, 1, 64, INIT_CAP_ROOT_CNODE, test_untyped, 64,
              CAP_RIGHTS_ALL);

  printf ("CDT Test cnode contents:\n");
  cnode_debug_print (test_cnode);

  struct thread_data td = {
    .elf_header = cdt_test_elf,
    .untyped = main_untyped, // Use main untyped for thread infrastructure
    .scratch_vspace = INIT_CAP_INIT_VSPACE,
    .cspace_root = test_cnode,
    .name = "cdt_test",
  };

  int err = spawn_thread (&td);
  if (err)
    printf ("Error spawning CDT test: %d\n", err);
  else
    {
      printf ("Successfully spawned CDT test\n");
      tcb_resume (td.tcb);
    }
}

cptr_t
spawn_pci_manager (cptr_t untyped, cptr_t port)
{
  const void *pci_manager_elf = find_tar_entry (bi->initrd, "pci_manager");
  if (!pci_manager_elf)
    {
      printf ("Could not find pci_manager elf\n");
      return 0;
    }

  int err = 0;

  // Create a cnode for the PCI manager (4 slots: endpoint, port, cnode, tmp)
  const cptr_t pci_cnode = allocate_with_size (untyped, CAP_CNODE, 1, 4);

  // pci_endpoint_cap (slot 0)
  const cptr_t pci_endpoint = allocate (untyped, CAP_ENDPOINT, 1);

  cnode_copy (pci_cnode, PCI_ENDPOINT_CAP, 64, INIT_CAP_ROOT_CNODE,
              pci_endpoint, 64, CAP_RIGHTS_ALL);

  // pci_port_cap (slot 1)
  cnode_copy (pci_cnode, PCI_PORT_CAP, 64, INIT_CAP_ROOT_CNODE, port, 64,
              CAP_RIGHTS_ALL);

  // pci_cnode_cap (slot 2)
  cnode_copy (pci_cnode, PCI_CNODE_CAP, 64, INIT_CAP_ROOT_CNODE, pci_cnode, 64,
              CAP_RIGHTS_ALL);

  // pci_tmp_cap (slot 3) - left empty for now

  struct thread_data td = {
    .elf_header = pci_manager_elf,
    .untyped = untyped,
    .scratch_vspace = INIT_CAP_INIT_VSPACE,
    .cspace_root = pci_cnode,
    .name = "pci_manager",
  };

  err = spawn_thread (&td);

  if (err)
    {
      printf ("Error creating pci_manager process: %d\n", err);
      return 0;
    }

  printf ("Successfully created pci_manager process\n");
  tcb_resume (td.tcb);
  return pci_endpoint;
}

word_t
calc_add (cptr_t calculator_endpoint, word_t a, word_t b)
{
  set_mr (0, a);
  set_mr (1, b);
  const message_info_t info = new_message_info (CALCULATOR_ADD, 0, 0, 2);
  call (calculator_endpoint, info, nullptr);
  return get_mr (0);
}

void
calculate_fibonacci_numbers (cptr_t calculator_endpoint, word_t up_to)
{
  word_t a = 0, b = 1;

  printf ("Fibonacci: ");
  while (true)
    {
      const word_t tmp = calc_add (calculator_endpoint, a, b);
      a = b;
      b = tmp;

      printf ("%lu ", a);

      if (a > up_to)
        break;
    }
  printf ("\n");
}

void
print_to_serial (cptr_t serial_endpoint, const char *message)
{
  for (const char *c = message; *c; c++)
    {
      set_mr (0, *c);
      message_info_t info = new_message_info (1, 0, 0, 1);
      send (serial_endpoint, info);
    }
}

void
serial_capitalization_server (cptr_t serial_endpoint,
                              cptr_t serial_notification)
{
  while (true)
    {
      message_info_t info = read_serial (serial_endpoint, serial_notification);
      const size_t regs = message_length (info);

      if (regs == 0)
        continue;

      printf ("Serial: ");

      for (size_t i = 0; i < regs; i++)
        {
          const uint8_t byte = get_mr (i);
          printf ("%c", byte);

          if (byte >= 'a' && byte <= 'z')
            set_mr (i, byte + 'A' - 'a');
        }

      info = new_message_info (SERIAL_DRIVER_WRITE, 0, 0, regs);
      send (serial_endpoint, info);
    }
}

void
enumerate_and_print_pci_devices (cptr_t pci_manager_endpoint)
{
  // Call enumerate to get all PCI device addresses
  message_info_t info = new_message_info (PCI_MANAGER_ENUMERATE, 0, 0, 0);
  info = call (pci_manager_endpoint, info, nullptr);

  const size_t num_devices = message_length (info);
  uint32_t addrs[num_devices];
  for (size_t i = 0; i < num_devices; i++)
    addrs[i] = get_mr (i) >> 32;

  if (num_devices == 0)
    {
      printf ("No PCI devices found\n");
      return;
    }

  printf ("\nEnumerating %zu PCI device(s):\n\n", num_devices);

  for (size_t i = 0; i < num_devices; i++)
    {
      const uint32_t pci_address = addrs[i];

      // Call device_info to get the first 64 bytes of config space
      set_mr (0, pci_address);
      info = new_message_info (PCI_MANAGER_DEVICE_INFO, 0, 0, 1);
      info = call (pci_manager_endpoint, info, nullptr);

      // Parse config space from returned MRs (8 qwords = 64 bytes)
      // Convert qwords back to dwords for easier access
      uint32_t config[16];
      for (int j = 0; j < 8; j++)
        {
          const uint64_t qword = get_mr (j);
          config[j * 2] = qword & 0xFFFFFFFF;
          config[j * 2 + 1] = qword >> 32;
        }

      // Extract fields from config space
      const uint16_t vendor_id = config[0] & 0xFFFF;
      const uint16_t device_id = config[0] >> 16;
      const uint16_t status = config[1] >> 16;
      const uint8_t revision_id = config[2] & 0xFF;
      const uint8_t prog_if = (config[2] >> 8) & 0xFF;
      const uint8_t subclass = (config[2] >> 16) & 0xFF;
      const uint8_t class_code = (config[2] >> 24) & 0xFF;
      const uint8_t irq = config[15] & 0xFF;

      printf ("PCI Device: %04X:%04X at addr %08X\n", vendor_id, device_id,
              pci_address);
      printf ("  Class: %02X, Subclass: %02X, Prog IF: %02X, Revision: %02X "
              "IRQ: %02X Status: %04X\n",
              class_code, subclass, prog_if, revision_id, irq, status);

      // Print BARs (at offsets 0x10-0x27, which is dwords 4-9)
      for (uint32_t bar_idx = 0; bar_idx < 6; bar_idx++)
        {
          const uint32_t bar = config[4 + bar_idx];
          if (bar == 0)
            continue;

          const bool is_io = bar & 1;
          if (is_io)
            {
              const uint32_t addr = bar & 0xFFFFFFFC;
              printf ("  - BAR%d: I/O %08X\n", bar_idx, addr);
            }
          else
            {
              const uint32_t addr = bar & 0xFFFFFFF0;
              printf ("  - BAR%d: Mem %08X\n", bar_idx, addr);
            }
        }
      printf ("\n");
    }
}

void *
map_framebuffer (cptr_t untyped)
{
  const struct limine_framebuffer fbinfo = bi->framebuffer_info;
  const size_t fbsize = fbinfo.height * fbinfo.width * fbinfo.bpp / 8;
  const uintptr_t fbaddr = (uintptr_t)fbinfo.address - 0xFFFF800000000000;

  cptr_t fbuntyped = 0;

  for (size_t i = 0; i < bi->n_untypeds; i++)
    {
      if (!bi->untypeds[i].is_device)
        continue;
      if (bi->untypeds[i].base == fbaddr)
        {
          fbuntyped = INIT_CAP_FIRST_UNTYPED + i;
          break;
        }
    }

  printf ("untyped = %zX\n", untyped);
  printf ("fbuntyped = %zX\n", fbuntyped);

  if (fbuntyped == 0)
    {
      printf ("Failed to find untyped for framebuffer\n");
      return nullptr;
    }

  for (size_t page_offset = 0; page_offset < fbsize; page_offset += 0x1000)
    {
      uintptr_t addr = fbaddr + page_offset;
      cptr_t cptr = cptr_alloc ();
      int err = untyped_retype (fbuntyped, CAP_X86_64_PAGE, 12,
                                INIT_CAP_ROOT_CNODE, INIT_CAP_ROOT_CNODE, 64,
                                cptr, 1);
      if (err != NO_ERROR)
        {
          printf ("Error: could not allocate framebuffer page: (%s)\n",
                  error_string (err));
          return nullptr;
        }

      err = map_page (untyped, INIT_CAP_INIT_VSPACE, cptr, addr);
      if (err != NO_ERROR)
        {
          printf ("Error: could not map framebuffer page: (%s)\n",
                  error_string (err));
          return nullptr;
        }
    }

  return (void *)fbaddr;
}

void
clear_fb (uint32_t *framebuffer, uint32_t color)
{
  const size_t width = bi->framebuffer_info.width;
  const size_t height = bi->framebuffer_info.height;

  for (size_t row = 0; row < height; row++)
    for (size_t col = 0; col < width; col++)
      framebuffer[row * width + col] = color;
}

void
draw_square (uint32_t *framebuffer, size_t r, size_t c, size_t w, size_t h,
             uint32_t color)
{
  const size_t width = bi->framebuffer_info.width;

  for (size_t row = r; row < r + h; row++)
    for (size_t col = c; col < c + w; col++)
      framebuffer[row * width + col] = color;
}

void
draw_circle (uint32_t *framebuffer, size_t r, size_t c, size_t radius,
             uint32_t color)
{
  const size_t width = bi->framebuffer_info.width;

  for (size_t row = r - radius; row < r + radius; row++)
    for (size_t col = c - radius; col < c + radius; col++)
      {
        if (row < 0 || row >= bi->framebuffer_info.height)
          continue;
        if (col < 0 || col >= bi->framebuffer_info.width)
          continue;

        const size_t rr = row - r;
        const size_t cc = col - c;
        if (rr * rr + cc * cc < radius * radius)
          framebuffer[row * width + col] = color;
      }
}

int
main (void *boot_info)
{
  bi = boot_info;
  cptr_alloc_init (bi);

  print_bootinfo_information ();

  setup_memory_information (bi);

  const word_t untyped = INIT_CAP_FIRST_UNTYPED + untypeds[0].index;
  const word_t test_untyped = INIT_CAP_FIRST_UNTYPED + untypeds[1].index;

  printf ("Largest untyped: %lX\n", untyped);
  printf ("CDT test untyped: %lX (size: %d bits)\n", test_untyped,
          untypeds[1].size_bits);

  const cptr_t pci_io_port = cptr_alloc ();
  x86_64_io_port_control_issue (INIT_CAP_IO_PORT_CONTROL, 0xCF8, 0xCFF,
                                INIT_CAP_ROOT_CNODE, pci_io_port, 64);

  // Spawn PCI manager and enumerate devices
  const cptr_t pci_manager_endpoint = spawn_pci_manager (untyped, pci_io_port);
  if (pci_manager_endpoint)
    enumerate_and_print_pci_devices (pci_manager_endpoint);

  const cptr_t e9_io_port = cptr_alloc ();
  x86_64_io_port_control_issue (INIT_CAP_IO_PORT_CONTROL, 0xE9, 0xE9,
                                INIT_CAP_ROOT_CNODE, e9_io_port, 64);
  print_to_e9 (e9_io_port, "Hello, E9 World!\n");

  // void *fb = map_framebuffer (untyped);
  // clear_fb (fb, 0xFFFFFFFF);
  // draw_square (fb, 100, 100, 100, 100, 0xFF);
  // draw_square (fb, 200, 200, 100, 100, 0xFF00);
  // draw_square (fb, 300, 300, 100, 100, 0xFF0000);
  // draw_square (fb, 400, 400, 100, 100, 0xFF000000);
  // draw_circle (fb, 150, 400, 50, 0xFF0000);

  const cptr_t calculator_endpoint = allocate (untyped, CAP_ENDPOINT, 1);
  const cptr_t serial_endpoint = allocate (untyped, CAP_ENDPOINT, 1);
  const cptr_t serial_ntfn = allocate (untyped, CAP_NOTIFICATION, 1);

  // Run CDT tests first (uses main untyped for infra, test_untyped for
  // testing)
  spawn_cdt_test (untyped, test_untyped);

  spawn_calculator_thread (untyped, calculator_endpoint);
  spawn_serial_driver (untyped, serial_endpoint, serial_ntfn);

  print_to_serial (serial_endpoint, "Hello, Serial World!\n");

  calculate_fibonacci_numbers (calculator_endpoint, 10'000);
  serial_capitalization_server (serial_endpoint, serial_ntfn);

  unreachable ();
}
