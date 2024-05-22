#include "sys/syscall.h"
#include "../arch/x86_64/x86_64.h"
#include "assert.h"
#include "stdio.h"
#include "sys/mem.h"
#include "sys/per_cpu.h"
#include "sys/task.h"

uintptr_t
do_syscall (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
            uintptr_t a4, uintptr_t a5, int syscall_number, frame_t *f)
{
  (void)a2, (void)a3, (void)a4, (void)a5;

  uintptr_t ret = 0;

  if (syscall_number != 1)
    printf ("Task %#lx ", (uintptr_t)this_task & 0xff);

  switch (syscall_number)
    {
    case 0:
      printf ("Exit ()\n");
      kill_task (this_task);
      schedule ();
      UNREACHABLE ();
    case 1:
      // printf ("Write (str: %#lx, len: %lu)\n", syscall_number, a0,
      //         a1);
      // printf ("  -> \"%.*s\"\n", (int)a1, (const char *)a0);
      write_debug (nullptr, (const void *)a0, a1);
      break;
    case 2:
      printf ("Clone (fn: %#lx, stk: %#lx)\n", a0, a1);
      struct task *new_t = create_task_in_this_vm (a0, a1);
      make_task_runnable (new_t);
      schedule ();
      break;
    case 3:
      printf ("Yield ()\n");
      schedule ();
      break;
    case 4:
      printf ("Send (to: %#lx, msg: %#lx)\n", a0, a1);
      send_message ((struct task *)a0, a1);
      break;
    case 5:
      printf ("Receive ()\n");
      receive_message ();
      break;
    case 6:
      printf ("Create (vm: %lu, arg: %lu)\n", a0, a1);
      struct task *t = create_task_from_syscall (a0 != 0, a1);
      make_task_runnable (t);
      ret = (uintptr_t)t;
      break;
    case 7:
      printf ("Map (virt: %#lx, phys: %#lx, len: %li, flags: %li)\n", a0, a1,
              a2, a3);
      assert (a0 % PAGE_SIZE == 0);
      assert (a1 % PAGE_SIZE == 0);

      for (size_t i = 0; i < a2; i += PAGE_SIZE)
        add_vm_mapping (this_cpu->current_task->vm_root, a0 + i, a1 + i,
                        (int)a3);
      break;
    case 8:
      printf ("IO Out (port: %#lx, val: %#lx)\n", a0, a1);
      write_port_b (a0, a1);
      break;
    case 9:
      printf ("IO In (port: %#lx)\n", a0);
      return read_port_b (a0);
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);

      printf ("Unknown syscall\n");
      break;
    }

  return ret;
}
