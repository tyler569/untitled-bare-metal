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
struct cap root_untyped_capabilities[MAX_UNTYPED_ROOT_CAPS];
size_t root_untyped_cap_count = 0;

int create_objects (word_t type, word_t size_bits, struct cap *dest_slot_0,
                    word_t num_objects, uintptr_t usable_memory,
                    struct cap *parent_untyped);

message_info_t
untyped_retype (struct cap *slot, word_t type, word_t size_bits,
                struct cap *root, word_t node_index, uint8_t node_depth,
                word_t node_offset, word_t num_objects)
{
  struct cap *untyped = slot;
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

  struct cap *dest_cnode;
  TRY (lookup_cap_slot (root, node_index, node_depth, &dest_cnode));

  if (cap_size (dest_cnode) < node_offset + num_objects)
    return msg_range_error (0, cap_size (dest_cnode));

  struct cap *dest_slot_0 = cap_for (dest_cnode, node_offset, node_depth);

  uintptr_t untyped_paddr = (uintptr_t)cap_ptr (untyped);
  uintptr_t usable_memory = untyped_paddr + untyped_offset;

  create_objects (type, size_bits, dest_slot_0, num_objects, usable_memory,
                  untyped);

  untyped->badge = untyped_offset + total_size;

  return msg_ok (0);
}

int
create_objects (word_t type, word_t size_bits, struct cap *dest_slot_0,
                word_t num_objects, uintptr_t usable_memory,
                struct cap *parent_untyped)
{
  size_t obj_size = object_size (type, size_bits);

  for (word_t i = 0; i < num_objects; i++)
    {
      struct cap *dest_slot = dest_slot_0 + i;
      uintptr_t obj_paddr = usable_memory + i * obj_size;
      void *obj_ptr = (void *)direct_map_of (obj_paddr);
      memset (obj_ptr, 0, obj_size);

      // printf ("Creating object of type %s at %p\n", cap_type_string (type),
      //         obj_ptr);

      dest_slot->type = type;
      dest_slot->size_bits = size_bits;
      dest_slot->badge = 0;
      dest_slot->is_original = 1; // Original capability created from untyped
      cap_set_ptr (dest_slot, obj_ptr);

      // Insert into CDT to track derivation from untyped memory
      cdt_insert (dest_slot, parent_untyped, cap_next (parent_untyped));
    }

  return 0;
}
