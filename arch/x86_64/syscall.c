#include "stdio.h"
#include "sys/cdefs.h"
#include "x86_64.h"

void
init_syscall ()
{
  uint64_t star = (USER_FAKE_SYSRET_CS << 48) | (KERNEL_CS << 32);
  uint64_t lstar = (uintptr_t)syscall_entry;
  uint64_t cstar = (uintptr_t)syscall_entry;
  uint64_t mask = FLAG_TF | FLAG_IF;
  uint64_t efer = read_msr (IA32_EFER);

  write_msr (IA32_STAR, star);
  write_msr (IA32_LSTAR, lstar);
  write_msr (IA32_CSTAR, cstar);
  write_msr (IA32_FMASK, mask);
  write_msr (IA32_EFER, efer | IA32_EFER_SCE);
}

USED uintptr_t
c_syscall_entry (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
                 uintptr_t a4, uintptr_t a5, int syscall_number)
{
  printf ("Syscall (num: %i, a0: %#lx, a1: %#lx)\n", syscall_number, a0, a1);

  switch (syscall_number)
    {
    case 0:
      printf ("exit syscall\n");
      break;
    case 1:
      printf ("%.*s\n", (int)a1, (const char *)a0);
      break;
    default:
      printf ("Unknown syscall\n");
      break;
    }

  return 0;
}
