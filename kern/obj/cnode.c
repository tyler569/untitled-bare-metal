#include "kern/obj/cnode.h"
#include "kern/cap.h"
#include "kern/syscall.h"

error_t
cnode_debug_print (cap_t obj)
{
  cte_t *cte = cap_ptr (obj);
  size_t size = cap_size (obj);

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
      printf ("  %zu: %s\n", i, cap_type_string (type));
    }

  return no_error;
}
