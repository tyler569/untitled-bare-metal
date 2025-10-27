#include "kern/obj/cnode.h"
#include "assert.h"
#include "kern/cap.h"
#include "kern/syscall.h"

// Low-level lookup that returns error_t without touching IPC buffer
error_t
lookup_cap_slot_raw (struct cap *cspace_root, word_t index, word_t depth,
                     struct cap **out)
{
  *out = nullptr;
  assert_eq (depth, 64); // for now

  if (cap_type (cspace_root) != cap_cnode)
    return invalid_root;

  struct cap *slots = cap_ptr (cspace_root);
  size_t length = cap_size (cspace_root);
  if (index >= length)
    return range_error;

  *out = &slots[index];
  return no_error;
}

// High-level wrapper that formats errors into IPC buffer
message_info_t
lookup_cap_slot (struct cap *cspace_root, word_t index, word_t depth,
                 struct cap **out)
{
  error_t err = lookup_cap_slot_raw (cspace_root, index, depth, out);

  if (err == invalid_root)
    return msg_invalid_root ();

  if (err == range_error)
    {
      size_t length = cap_size (cspace_root);
      return msg_range_error (0, length);
    }

  return msg_ok (0);
}

message_info_t
cnode_debug_print (struct cap *obj)
{
  struct cap *slots = cap_ptr (obj);
  size_t size = cap_size (obj);

  printf ("CNode %p\n", slots);
  printf ("  size:: %lu\n", size);

  for (size_t i = 0; i < size; i++)
    {
      word_t type = cap_type (&slots[i]);
      if (type == cap_null)
        continue;
      if (type >= max_cap_type)
        {
          printf ("  %zu: Invalid cap type %lu\n", i, type);
          continue;
        }
      printf ("  %zu: %s\n", i, cap_type_string (type));
    }

  return msg_ok (0);
}

message_info_t
cnode_copy (struct cap *obj, word_t dst_offset, uint8_t dst_depth,
            struct cap *root, word_t src_offset, uint8_t src_depth,
            cap_rights_t rights)
{
  struct cap *dst;
  TRY (lookup_cap_slot (obj, dst_offset, dst_depth, &dst));

  struct cap *src;
  TRY (lookup_cap_slot (root, src_offset, src_depth, &src));

  // Check that destination is empty
  if (cap_type (dst) != cap_null)
    return msg_delete_first ();

  // Copy capability data with rights restriction
  copy_cap_data (dst, src, rights);

  // Derived capabilities are not original (except badged endpoints - see mint)
  dst->is_original = 0;

  // Insert into capability derivation tree
  // For untyped caps, no CDT insertion (they don't have siblings)
  if (cap_type (src) != cap_untyped)
    cdt_insert (dst, src, cap_next (src));

  return msg_ok (0);
}

message_info_t
cnode_delete (struct cap *obj, word_t offset, uint8_t depth)
{
  struct cap *cap;
  TRY (lookup_cap_slot (obj, offset, depth, &cap));

  // Use cap_delete which checks for children
  error_t err = cap_delete (cap);
  if (err == revoke_first)
    return msg_revoke_first ();
  if (err != no_error)
    return new_message_info (err, 0, 0, 0);

  return msg_ok (0);
}

message_info_t
cnode_revoke (struct cap *obj, word_t offset, uint8_t depth)
{
  struct cap *cap;
  TRY (lookup_cap_slot (obj, offset, depth, &cap));

  // Recursively revoke all derived capabilities
  error_t err = cap_revoke (cap);
  if (err != no_error)
    return new_message_info (err, 0, 0, 0);

  return msg_ok (0);
}

message_info_t
cnode_mint (struct cap *obj, word_t dst_offset, uint8_t dst_depth,
            struct cap *root, word_t src_offset, uint8_t src_depth,
            cap_rights_t rights, word_t badge)
{
  struct cap *dst;
  TRY (lookup_cap_slot (obj, dst_offset, dst_depth, &dst));

  struct cap *src;
  TRY (lookup_cap_slot (root, src_offset, src_depth, &src));

  // Check that destination is empty
  if (cap_type (dst) != cap_null)
    return msg_delete_first ();

  // Only endpoints and notifications can be minted
  if (cap_type (src) != cap_endpoint && cap_type (src) != cap_notification)
    return msg_invalid_argument (4);

  // Source must not already be badged
  if (src->badge)
    return msg_invalid_argument (4);

  // Copy capability data with rights restriction
  copy_cap_data (dst, src, rights);

  // Set the badge
  dst->badge = badge;

  // Badged endpoints/notifications are treated as "original" capabilities
  // They can have their own derived children (seL4 semantics)
  dst->is_original = 1;

  // Insert into capability derivation tree
  cdt_insert (dst, src, cap_next (src));

  return msg_ok (0);
}
