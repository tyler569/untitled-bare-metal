#include "x86_64.h"
#include <kernel.h>
#include <stdio.h>

const char *interrupt_acronyms[]
    = { "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
        "#DF", "CSO", "#TS", "#NP", "#SS", "#GP", "#PF", "R15",
        "#MF", "#AC", "#MC", "#XF", "#VE", "#CP", "R22", "R23",
        "R24", "R25", "R26", "R27", "#HV", "#VC", "#SX", "R31" };

void
print_interrupt_info (frame_t *f)
{
  printf ("Interrupt %lu (%s) @ %lx\n", f->int_no,
          interrupt_acronyms[f->int_no], f->rip);

  switch (f->int_no)
    {
    case 8:
      printf ("Double fault\n");
      halt_forever ();
    case 14:
      printf ("Page fault at %lx\n", read_cr2 ());
      printf ("Error code: %lx\n", f->err_code);
    }
}