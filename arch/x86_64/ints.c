#include "assert.h"
#include "kern/arch.h"
#include "kern/mem.h"
#include "stdio.h"
#include "string.h"
#include "x86_64.h"

const char *interrupt_acronyms[]
    = { "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
        "#DF", "CSO", "#TS", "#NP", "#SS", "#GP", "#PF", "R15",
        "#MF", "#AC", "#MC", "#XF", "#VE", "#CP", "R22", "R23",
        "R24", "R25", "R26", "R27", "#HV", "#VC", "#SX", "R31" };

void
print_interrupt_info (frame_t *f)
{
  if (f->int_no < 32)
    printf ("Interrupt %lu (%s) @ %#lx\n", f->int_no,
            interrupt_acronyms[f->int_no], f->rip);
  else
    printf ("Interrupt %lu @ %#lx\n", f->int_no, f->rip);

  switch (f->int_no)
    {
    case 2:
      printf ("NMI\n");
      break;
    case 3:
      printf ("Debug trap\n");
      print_frame (f);
      print_backtrace (f);
      break;
    case 8:
      printf ("Double fault\n");
      print_frame (f);
      print_backtrace (f);
      break;
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
    case 32 ... 48:
      bool handle_irq (word_t irq);
      if (!handle_irq (f->int_no - 32))
        send_eoi (f->int_no);

      return;
    case 255:
      return;
    default:
      print_backtrace (f);
    }

  halt_forever ();
}
