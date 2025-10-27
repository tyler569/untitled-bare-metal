#include "kern/cap.h"
#include "kern/ipc.h"
#include "kern/mem.h"
#include "kern/methods.h"
#include "kern/obj/cnode.h"
#include "kern/obj/tcb.h"
#include "kern/size.h"
#include "kern/syscall.h"
#include "stdio.h"
#include "string.h"
#include "sys/types.h"

#define MAX_UNTYPED_ROOT_CAPS 256
cap_t root_untyped_capabilities[MAX_UNTYPED_ROOT_CAPS];
size_t root_untyped_cap_count = 0;

int create_objects (cte_t *untyped, word_t type, word_t size_bits, cte_t *dest_slot_0,
                    word_t num_objects, uintptr_t usable_memory);

message_info_t
untyped_retype (cte_t *slot, word_t type, word_t size_bits, cte_t *root,
                word_t node_index, uint8_t node_depth, word_t node_offset,
                word_t num_objects)
{
  cap_t *untyped = &slot->cap;
  size_t untyped_size = BIT (untyped->size_bits);
  size_t obj_size = object_size (type, size_bits);
  size_t total_size = obj_size * num_objects;
  size_t untyped_offset = ALIGN_UP (untyped->badge, obj_size);
  size_t available_memory = untyped_size - untyped_offset;

  if (available_memory < total_size)
    {
      printf ("not enough memory to make %lu objects of type %lu (size %lu "
              "bytes). We have %lu bytes available.\n",
              num_objects, type, obj_size, available_memory);
      return msg_not_enough_memory (available_memory);
    }

  cte_t *dest_cnode;
  TRY (lookup_cap_slot (root, node_index, node_depth, &dest_cnode));

  // TODO: return an error in this case/ invalid argument or smth
  assert (cap_type (dest_cnode) == cap_cnode);

  if (cap_size (dest_cnode) < node_offset + num_objects)
    return msg_range_error (0, cap_size (dest_cnode));

  cte_t *dest_slot_0 = cte_for (dest_cnode, node_offset, node_depth);
  assert (dest_slot_0);

  uintptr_t untyped_paddr = (uintptr_t)cap_ptr (*untyped);
  uintptr_t usable_memory = untyped_paddr + untyped_offset;

  create_objects (slot, type, size_bits, dest_slot_0, num_objects, usable_memory);

  untyped->badge = untyped_offset + total_size;

  return msg_ok (0);
}

int
create_objects (cte_t *untyped, word_t type, word_t size_bits,
                cte_t *dest_slot_0, word_t num_objects, uintptr_t usable_memory)
{
  size_t obj_size = object_size (type, size_bits);

  for (word_t i = 0; i < num_objects; i++)
    {
      cte_t *dest_slot = dest_slot_0 + i;
      uintptr_t obj_paddr = usable_memory + i * obj_size;
      void *obj_ptr = (void *)direct_map_of (obj_paddr);
      memset (obj_ptr, 0, obj_size);

      // printf ("Creating object of type %s at %p\n", cap_type_string (type),
      //         obj_ptr);

      dest_slot->cap.type = type;
      dest_slot->cap.size_bits = size_bits;
      dest_slot->cap.badge = 0;
      cap_set_ptr (dest_slot, obj_ptr);

      insert (dest_slot, untyped, untyped->next);
    }

  return 0;
}
