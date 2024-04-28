#include "kernel.h"
#include "limine.h"
#include "stdio.h"
#include "sys/arch.h"
#include "sys/cdefs.h"
#include "sys/mem.h"
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
  init_kmem_alloc ();
  init_aps ();

  kernel_main ();

  halt_forever ();
}

USED void
c_exception_entry (frame_t *f)
{
  print_interrupt_info (f);
}

USED void
c_interrupt_entry (frame_t *f)
{
  print_interrupt_info (f);
}
