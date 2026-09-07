#include "kern/arch/x86_64/exports.h"
#include "kern/per_cpu.h"
#include "stdio.h"

void
print_backtrace (frame_t *f)
{
  if (this_cpu->printing_backtrace)
    {
      printf ("Faulted in backtrace\n");
      return;
    }

  this_cpu->printing_backtrace = true;

  printf ("Backtrace:\n");

  uintptr_t rbp = f->rbp;
  uintptr_t rip = f->rip;
  int frames = 0;

  while (rbp && frames++ < 25)
    {
      printf ("  frame ip: %lX\n", rip);

      rip = *(uintptr_t *)(rbp + 8);
      rbp = *(uintptr_t *)rbp;
    }

  this_cpu->printing_backtrace = false;
}

void
print_frame (frame_t *f)
{
  printf ("rax %16lX rbx %16lX rcx %16lX rdx %16lX\n", f->rax, f->rbx, f->rcx,
          f->rdx);
  printf ("rsi %16lX rdi %16lX rbp %16lX rsp %16lX\n", f->rsi, f->rdi, f->rbp,
          f->rsp);
  printf (" r8 %16lX  r9 %16lX r10 %16lX r11 %16lX\n", f->r8, f->r9, f->r10,
          f->r11);
  printf ("r12 %16lX r13 %16lX r14 %16lX r15 %16lX\n", f->r12, f->r13, f->r14,
          f->r15);
  printf (" cs %16lX  ss %16lX\n", f->cs, f->ss);
  printf ("int %16lX err %16lX rip %16lX flg %16lX\n", f->int_no, f->err_code,
          f->rip, f->rflags);
}
