#include "sys/obj/untyped.h"
#include "kern/cap.h"
#include "kern/mem.h"
#include "kern/obj/tcb.h"
#include "kern/syscall.h"
#include "sys/types.h"

cap_t root_untyped_capabilities[MAX_UNTYPED_ROOT_CAPS];
size_t root_untyped_cap_count = 0;

int
invoke_untyped_cap (cap_t untyped, word_t method)
{
  if (cap_type (untyped) != cap_untyped)
    {
      return_ipc_error (illegal_operation, 0);
      return -1;
    }

  switch (method)
    {
    case untyped_retype:
    default:
      return_ipc_error (illegal_operation, 0);
      return -1;
    }
}
