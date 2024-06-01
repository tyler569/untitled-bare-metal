#include "kern/syscall.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/obj/endpoint.h"
#include "kern/obj/tcb.h"
#include "kern/per_cpu.h"
#include "stdio.h"

#include "kern/kernel_method_stubs.c"
#include "kern/methods.h"
#include "kern/syscall_dispatch.c"

uintptr_t
do_syscall (uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3,
            uintptr_t a4, uintptr_t a5, int syscall_number, frame_t *f)
{
  (void)a2, (void)a3, (void)a4, (void)a5, (void)f;

  uintptr_t ret = 0;

  if (syscall_number != sys_debug_write)
    printf ("Task %#lx ", (uintptr_t)this_tcb & 0xff);

  if (syscall_number == sys_exit)
    {
      printf ("sys_exit ()\n");
      kill_tcb (this_tcb);
      schedule ();
      unreachable ();
    }
  else if (syscall_number == sys_debug_write)
    {
      write_debug (nullptr, (const void *)a0, a1);
      return no_error;
    }

  cptr_t cptr = (cptr_t)a0;
  message_info_t info = (message_info_t)a1;

  cap_t obj;
  error_t l = lookup_cap (this_tcb->cspace_root, cptr, 64, &obj);
  if (l != no_error)
    {
      return_ipc (l, 0);
      return l;
    }

  switch (syscall_number)
    {
    case sys_call:
      return dispatch_method (obj, info);
    case sys_send:
      printf ("sys_send (dest: %#lx, info: %#lx)\n", cptr, info);

      if (obj.type != cap_endpoint)
        {
          return_ipc (illegal_operation, 0);
          return illegal_operation;
        }

      return invoke_endpoint_send (obj, info);
    default:
      printf ("Syscall (num: %i, ?...)\n", syscall_number);
      return no_error;
    }

  return ret;
}
