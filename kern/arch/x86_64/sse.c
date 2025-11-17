#include "x86_64.h"

void
init_sse ()
{
  uint64_t cr0 = read_cr0 ();
  cr0 &= ~CR0_EM;  // Clear emulation
  cr0 |= CR0_MP;   // Set monitor coprocessor
  write_cr0 (cr0);

  uint64_t cr4 = read_cr4 ();
  cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;
  write_cr4 (cr4);
}
