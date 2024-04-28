#include "stdio.h"
#include "sys/arch.h"
#include "x86_64.h"

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