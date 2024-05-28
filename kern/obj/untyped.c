#include "sys/obj/untyped.h"
#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/mem.h"
#include "kern/obj/cnode.h"
#include "kern/obj/tcb.h"
#include "kern/size.h"
#include "kern/syscall.h"
#include "stdio.h"
#include "string.h"
#include "sys/types.h"

cap_t root_untyped_capabilities[MAX_UNTYPED_ROOT_CAPS];
size_t root_untyped_cap_count = 0;

int invoke_untyped_retype (cap_t untyped, word_t type, word_t size_bits,
                           cap_t dest_cnode, word_t index, word_t depth,
                           word_t num_objects);

int create_objects (word_t type, word_t size_bits, cte_t *dest_slot_0,
                    word_t num_objects, uintptr_t usable_memory);

int
invoke_untyped_cap (cap_t untyped, word_t method)
{
  if (cap_type (untyped) != cap_untyped)
    {
      return_ipc (illegal_operation, 0);
      return -1;
    }

  switch (method)
    {
    case untyped_retype:
      {
        int len = get_ipc_length ();
        if (len < 7)
          {
            return_ipc (truncated_message, 0);
            return -1;
          }

        word_t type = get_mr (0);
        word_t size_bits = get_mr (1);
        cptr_t dest_cnode_ptr = get_mr (2);
        word_t index = get_mr (3);
        word_t depth = get_mr (4);
        word_t offset = get_mr (5);
        word_t num_objects = get_mr (6);

        printf ("untyped_retype: type %lu, size_bits %lu, dest_cnode %lu, "
                "index %lu, depth %lu, offset %lu, num_objects %lu\n",
                type, size_bits, dest_cnode_ptr, index, depth, offset,
                num_objects);

        cap_t dest_cspace_root;
        cap_t dest_cnode;

        exception_t status = lookup_cap (this_tcb->cspace_root, dest_cnode_ptr,
                                         64, &dest_cspace_root);

        if (status != no_error)
          {
            return_ipc (status, 0);
            return -1;
          }

        status = lookup_cap (dest_cspace_root, index, 64, &dest_cnode);

        if (status != no_error)
          {
            return_ipc (status, 0);
            return -1;
          }

        return invoke_untyped_retype (untyped, type, size_bits, dest_cnode,
                                      offset, depth, num_objects);
      }
    default:
      return_ipc (illegal_operation, 0);
      return -1;
    }
}

int
invoke_untyped_retype (cap_t untyped, word_t type, word_t size_bits,
                       cap_t dest_cnode, word_t index, word_t depth,
                       word_t num_objects)
{
  size_t untyped_size = BIT (untyped.size_bits);
  size_t obj_size = object_size (type, size_bits);
  size_t total_size = obj_size * num_objects;
  size_t untyped_offset = ALIGN_UP (untyped.badge, obj_size);
  size_t available_memory = untyped_size - untyped_offset;

  if (available_memory < total_size)
    {
      printf ("not enough memory to make %lu objects of type %lu (size %lu "
              "bytes). We have %lu bytes available.\n",
              num_objects, type, obj_size, available_memory);
      return_ipc (not_enough_memory, 0);
      return -1;
    }

  if (cap_size (dest_cnode) < index + num_objects)
    {
      return_ipc (range_error, 0);
      return -1;
    }

  cte_t *dest_slot_0 = cte_for (dest_cnode, index, depth);

  uintptr_t untyped_paddr = (uintptr_t)cap_ptr (untyped);
  uintptr_t usable_memory = untyped_paddr + untyped_offset;

  create_objects (type, size_bits, dest_slot_0, num_objects, usable_memory);

  untyped.badge = untyped_offset + total_size;

  return_ipc (no_error, 0);

  return 0;
}

int
create_objects (word_t type, word_t size_bits, cte_t *dest_slot_0,
                word_t num_objects, uintptr_t usable_memory)
{
  size_t obj_size = object_size (type, size_bits);

  for (word_t i = 0; i < num_objects; i++)
    {
      cte_t *dest_slot = dest_slot_0 + i;
      uintptr_t obj_paddr = usable_memory + i * obj_size;
      void *obj_ptr = (void *)direct_map_of (obj_paddr);
      memset (obj_ptr, 0, obj_size);

      dest_slot->cap.type = type;
      dest_slot->cap.size_bits = size_bits;
      dest_slot->cap.badge = 0;
      cap_set_ptr (&dest_slot->cap, obj_ptr);
    }

  return 0;
}
