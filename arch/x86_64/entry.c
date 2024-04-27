#include "kernel.h"
#include "limine.h"
#include "x86_64.h"
#include <stdio.h>
#include <sys/cdefs.h>

USED void
kernel_entry ()
{
  init_gdt ();
  init_idt ();

  init_page_mmap ();

  init_aps ();

  kernel_main ();

  halt_forever ();
}

void
ap_entry (struct limine_smp_info *)
{
  printf ("AP started\n");

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

USED void
c_syscall_entry ()
{
  printf ("Syscall\n");
}
