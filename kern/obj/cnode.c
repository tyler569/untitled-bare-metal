#include "kern/obj/cnode.h"
#include "kern/cap.h"
#include "kern/syscall.h"

cte_t *
lookup_cap_slot (cte_t *cspace_root, word_t index, word_t depth, error_t *err)
{
  assert_eq (depth, 64); // for now
  // TODO: this should just return an error
  assert_eq (cap_type (cspace_root->cap), cap_cnode);

  cte_t *cte = cap_ptr (cspace_root->cap);
  size_t length = cap_size (cspace_root->cap);
  if (index >= length)
    printf ("index: %lu, length: %lu\n", index, length);
  assert (index < length);

  *err = no_error;
  return &cte[index];
}

error_t
cnode_debug_print (cap_t *obj)
{
  cte_t *cte = cap_ptr (*obj);
  size_t size = cap_size (*obj);

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

message_info_t
cnode_copy (cte_t *obj, word_t dst_offset, uint8_t dst_depth, cte_t *root,
            word_t src_offset, uint8_t src_depth, cap_rights_t rights)
{
  error_t err;
  cte_t *dst = lookup_cap_slot (obj, dst_offset, dst_depth, &err);
  if (err != no_error)
    return return_ipc (err, 0);

  cte_t *src = lookup_cap_slot (root, src_offset, src_depth, &err);
  if (err != no_error)
    return return_ipc (err, 0);

  copy_cap (dst, src, rights);

  return return_ipc (no_error, 0);
}

message_info_t
cnode_delete (cte_t *obj, word_t offset, uint8_t depth)
{
  error_t err;
  cte_t *cte = lookup_cap_slot (obj, offset, depth, &err);
  if (err != no_error)
    return return_ipc (err, 0);

  // TODO: distribution tree validity
  cte->cap = cap_null_new ();

  return return_ipc (no_error, 0);
}

message_info_t
cnode_mint (cte_t *obj, word_t dst_offset, uint8_t dst_depth, cte_t *root,
            word_t src_offset, uint8_t src_depth, cap_rights_t rights,
            word_t badge)
{
  error_t err;
  cte_t *dst = lookup_cap_slot (root, dst_offset, dst_depth, &err);
  if (err != no_error)
    return return_ipc (err, 0);

  cte_t *src = lookup_cap_slot (obj, src_offset, src_depth, &err);
  if (err != no_error)
    return return_ipc (err, 0);

  if (cap_type (src) != cap_endpoint && cap_type (src) != cap_notification)
    return return_ipc (invalid_argument, 0);

  if (src->cap.badge)
    return return_ipc (invalid_argument, 0);

  copy_cap (dst, src, rights);
  dst->cap.badge = badge;

  return return_ipc (no_error, 0);
}
