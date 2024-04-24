#include "kernel/main.h"
#include "x86_64.h"
#include <kernel/arch.h>
#include <stdio.h>
#include <sys/cdefs.h>

[[noreturn]] USED void
kernel_entry ()
{
  init_gdt ();
  init_idt ();

  kernel_main ();
}

USED void
c_interrupt_entry (frame_t *f)
{
  printf ("Interrupt %lu @ %lx\n", f->int_no, f->rip);
}

USED void
c_syscall_entry ()
{
  printf ("Syscall\n");
}
