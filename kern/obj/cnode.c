#include "kern/obj/cnode.h"
#include "assert.h"
#include "kern/cap.h"
#include "kern/syscall.h"

// Low-level lookup that returns error_t without touching IPC buffer
error_t
lookup_cap_slot_raw (cte_t *cspace_root, word_t index, word_t depth,
                     cte_t **out)
{
  *out = nullptr;
  assert_eq (depth, 64); // for now

  if (cap_type (cspace_root->cap) != cap_cnode)
    return invalid_root;

  cte_t *cte = cap_ptr (cspace_root->cap);
  size_t length = cap_size (cspace_root->cap);
  if (index >= length)
    return range_error;

  *out = &cte[index];
  return no_error;
}

// High-level wrapper that formats errors into IPC buffer
message_info_t
lookup_cap_slot (cte_t *cspace_root, word_t index, word_t depth, cte_t **out)
{
  error_t err = lookup_cap_slot_raw (cspace_root, index, depth, out);

  if (err == invalid_root)
    return msg_invalid_root ();

  if (err == range_error)
    {
      size_t length = cap_size (cspace_root->cap);
      return msg_range_error (0, length);
    }

  return msg_ok (0);
}

message_info_t
cnode_debug_print (cte_t *obj)
{
  cte_t *cte = cap_ptr (obj->cap);
  size_t size = cap_size (obj->cap);

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

  return msg_ok (0);
}

message_info_t
cnode_copy (cte_t *obj, word_t dst_offset, uint8_t dst_depth, cte_t *root,
            word_t src_offset, uint8_t src_depth, cap_rights_t rights)
{
  cte_t *dst;
  TRY (lookup_cap_slot (obj, dst_offset, dst_depth, &dst));

  cte_t *src;
  TRY (lookup_cap_slot (root, src_offset, src_depth, &src));

  copy_cap (dst, src, rights);

  return msg_ok (0);
}

message_info_t
cnode_delete (cte_t *obj, word_t offset, uint8_t depth)
{
  cte_t *cte;
  TRY (lookup_cap_slot (obj, offset, depth, &cte));

  error_t err = delete (cte);

  return msg_err (err, 0);
}

message_info_t
cnode_mint (cte_t *obj, word_t dst_offset, uint8_t dst_depth, cte_t *root,
            word_t src_offset, uint8_t src_depth, cap_rights_t rights,
            word_t badge)
{
  cte_t *dst;
  TRY (lookup_cap_slot (obj, dst_offset, dst_depth, &dst));

  cte_t *src;
  TRY (lookup_cap_slot (root, src_offset, src_depth, &src));

  if (cap_type (src) != cap_endpoint && cap_type (src) != cap_notification)
    return msg_invalid_argument (4);

  if (src->cap.badge)
    return msg_invalid_argument (4);

  error_t err = mint (dst, src, badge, rights);

  return msg_err (err, 0);
}
