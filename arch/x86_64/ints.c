#include "stdio.h"
#include "string.h"
#include "sys/arch.h"
#include "sys/mem.h"
#include "x86_64.h"

#define INT_STACK_SIZE 8192

const char *interrupt_acronyms[]
    = { "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
        "#DF", "CSO", "#TS", "#NP", "#SS", "#GP", "#PF", "R15",
        "#MF", "#AC", "#MC", "#XF", "#VE", "#CP", "R22", "R23",
        "R24", "R25", "R26", "R27", "#HV", "#VC", "#SX", "R31" };

void
print_interrupt_info (frame_t *f)
{
  printf ("Interrupt %lu (%s) @ %#lx\n", f->int_no,
          interrupt_acronyms[f->int_no], f->rip);

  switch (f->int_no)
    {
    case 3:
      print_frame (f);
      return;
    case 13:
      printf ("General protection fault\n");
      print_frame (f);
      print_backtrace (f);
      break;
    case 14:
      printf ("Page fault at %#lx\n", read_cr2 ());
      printf ("Error code: %#lx\n", f->err_code);
      print_backtrace (f);
      break;
    default:
      print_backtrace (f);
    }

  halt_forever ();
}

void
init_int_stacks ()
{
  void *int_stack = kmem_alloc (INT_STACK_SIZE);
  void *nmi_stack = kmem_alloc (INT_STACK_SIZE);
  this_cpu->kernel_stack_top = (uintptr_t)int_stack + INT_STACK_SIZE;
  this_cpu->arch.tss.ist[0] = this_cpu->kernel_stack_top;
  this_cpu->arch.tss.ist[1] = (uintptr_t)nmi_stack + INT_STACK_SIZE;
}

frame_t *
new_user_frame (uintptr_t rip, uintptr_t rsp)
{
  frame_t *f = kmem_alloc (sizeof (frame_t));
  memset (f, 0, sizeof (frame_t));

  f->cs = USER_CS;
  f->ss = USER_SS;
  f->rip = rip;
  f->rsp = rsp;
  return f;
}
