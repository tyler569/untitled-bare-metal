#include "kern/obj/cnode.h"
#include "kern/cap.h"
#include "kern/syscall.h"

void do_cnode_debug_print (cap_t cap);

int
invoke_cnode_cap (cap_t cap, message_info_t info)
{
  if (cap_type (cap) != cap_cnode)
    {
      return_ipc (illegal_operation, 0);
      return -1;
    }

  switch (get_message_label (info))
    {
    case cnode_debug_print:
      do_cnode_debug_print (cap);
      return 0;
    default:
      return_ipc (illegal_operation, 0);
      return -1;
    };

  return_ipc (no_error, 0);

  return 0;
}

void
do_cnode_debug_print (cap_t cap)
{
  cte_t *cte = cap_ptr (cap);
  size_t size = cap_size (cap);

  printf ("CNode %p\n", cte);
  printf ("  size:: %lu\n", size);

  for (size_t i = 0; i < size; i++)
    {
      word_t type = cap_type (cte[i].cap);
      if (type == cap_null)
        continue;
      if (type >= max_cap_type)
        {
          printf ("  %zu: Invalid cap type %lu\n", i, type);
          continue;
        }
      printf ("  %zu: %s\n", i, cap_type_strings[type]);
    }
}
