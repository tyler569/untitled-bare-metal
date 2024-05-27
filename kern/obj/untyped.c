#include "sys/obj/untyped.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/mem.h"
#include "kern/task.h"
#include "sys/types.h"

cap_t root_untyped_capabilities[MAX_UNTYPED_ROOT_CAPS];
size_t root_untyped_cap_count = 0;

int
invoke_untyped_cap ()
{
  cptr_t untyped_cptr = get_extra_cap (0);
  cap_t untyped;
  exception_t status
      = lookup_cap (this_task->cspace_root, untyped_cptr, 64, &untyped);

  if (status != no_error)
    {
      return_ipc_error (status, 0);
      return -1;
    }

  if (cap_type (untyped) != CAP_UNTYPED)
    {
      return_ipc_error (illegal_operation, 0);
      return -1;
    }

  switch (get_ipc_label ())
    {
    case untyped_retype:
      {
      }
    default:
      return_ipc_error (illegal_operation, 0);
      return -1;
    }
}
