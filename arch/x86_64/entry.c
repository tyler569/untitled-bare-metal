#include "kern/arch.h"
#include "kern/kernel.h"
#include "kern/mem.h"
#include "limine.h"
#include "sys/cdefs.h"
#include "x86_64.h"

LIMINE_BASE_REVISION (1)

// The kernel entrypoint, called by the bootloader.
// This function is set as the entry on the kernel ELF file.
USED void
kernel_entry ()
{
  init_bsp_gdt ();
  init_idt ();
  init_syscall ();
  init_page_mmap ();
  init_int_stacks ();
  // init_aps ();

  kernel_main ();

  halt_forever ();
}

USED void
c_interrupt_entry (frame_t *f)
{
  save_frame_on_tcb (f);
  print_interrupt_info (f);
  clear_frame_on_tcb (f);
}
