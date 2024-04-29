#include "stdio.h"
#include "sys/arch.h"
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
c_syscall_entry (uintptr_t a0, uintptr_t a1, uintptr_t, uintptr_t, uintptr_t,
                 uintptr_t, int syscall_number, frame_t *f)
{
  printf ("Syscall (num: %i, a0: %#lx, a1: %#lx)\n", syscall_number, a0, a1);

  save_frame_on_task (f);

  switch (syscall_number)
    {
    case 0:
      printf ("Exit (num: %i)\n", syscall_number);
      kill_task (this_cpu->current_task);
      schedule ();
      break;
    case 1:
      printf ("Write (num: %i, str: %#lx, len: %lu)\n", syscall_number, a0,
              a1);
      printf ("  -> \"%.*s\"\n", (int)a1, (const char *)a0);
      break;
    case 2:
      printf ("Clone (num: %i, fn: %#lx, stk: %#lx)\n", syscall_number, a0,
              a1);
      struct task *new_t = create_task_in_this_vm (a0, a1);
      make_task_runnable (new_t);
      schedule ();
      break;
    case 3:
      printf ("Yield (num: %i)\n", syscall_number);
      schedule ();
      break;
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);
      printf ("Unknown syscall\n");
      break;
    }

  clear_frame_on_task (f);

  return 0;
}
