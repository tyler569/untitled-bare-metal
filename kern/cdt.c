#include "assert.h"
#include "kern/cap.h"
#include "kern/mem.h"
#include "kern/obj/untyped.h"
#include "kern/syscall.h"
#include "sys/syscall.h"

void
insert_cte_after (struct cte *new, struct cte *after)
{
  struct cte *next = after->next;

  if (next)
    next->prev = new;
  after->next = new;

  new->next = next;
  new->prev = after;
}

void
unlink_cte (struct cte *del)
{
  if (del->prev)
    del->prev->next = del->next;
  if (del->next)
    del->next->prev = del->prev;
  del->next = nullptr;
  del->prev = nullptr;
}

message_info_t
copy_cap (struct cte *dest, struct cte *src, cap_rights_t rights_mask)
{
  if (cap_type (dest) != CAP_NULL)
    return msg_delete_first ();

  if (cap_type (src) == CAP_UNTYPED && src->cap.badge)
    return msg_revoke_first ();

  dest->cap = src->cap;

  if (cap_type (src) != CAP_UNTYPED)
    dest->cap.is_original = false;

  cap_set_rights (dest, cap_rights (src) & rights_mask);

  insert_cte_after (dest, src);

  return msg_ok (0);
}

message_info_t
mint_cap (struct cte *dest, struct cte *src, unsigned long badge,
          cap_rights_t rights_mask)
{
  if (cap_type (dest) != CAP_NULL)
    return msg_delete_first ();

  if (cap_type (src) != CAP_ENDPOINT && cap_type (src) != CAP_NOTIFICATION)
    return msg_illegal_operation ();

  if (src->cap.badge != 0)
    return msg_illegal_operation ();

  dest->cap = src->cap;
  dest->cap.badge = badge;

  // minted badged endpoints are marked original, notifications are not
  if (cap_type (src) == CAP_NOTIFICATION)
    dest->cap.is_original = false;

  cap_set_rights (dest, cap_rights (src) & rights_mask);

  insert_cte_after (dest, src);

  return msg_ok (0);
}

/*
 * This function is only called to determine the end of the list of derived
 * capabilities off of a specific original cap; it does not establish a
 * total parent->child relationship for all capabilities, so this is only
 * valid for the run of caps in the CDT list such that
 *  [original] -> derived -> derived -> derived -> [original]
 * -- this will return `true` for all derived caps, then return false when
 * no longer looking at a derived cap of the original.
 *
 * The rules are as follows
 *
 * only ->is_original caps can be parents
 * untyped are the parents of all caps whose pointers lie inside their region
 * otherwise, caps can only parent their own type
 *   (children can only be created with `untyped_retype`, `copy_cap`,
 *   and `mint_cap`)
 * and endpoints with badge 0 parent all derived caps, endpointed with badge
 *   nonzero parent all dervied caps with matching endpoint. The original
 *   badged cap does have ->is_original set.
 */
bool
is_child_cap (struct cte *c, struct cte *parent)
{
  assert (parent);

  if (!c)
    return false;
  if (!parent->cap.is_original)
    return false;
  if (parent->cap.type == CAP_UNTYPED)
    return untyped_contains (parent, c);
  if (c->cap.type != parent->cap.type)
    return false;
  if (parent->cap.type == CAP_ENDPOINT)
    return parent->cap.badge == c->cap.badge || parent->cap.badge == 0;
  return true;
}

message_info_t
delete_cap (struct cte *c)
{
  revoke_cap (c);
  unlink_cte (c);

  if (c->cap.is_original)
    {
      // TODO: deletion operation on kernel object
    }

  c->cap = new_null_cap ();

  return msg_ok (0);
}

// revoke_cap takes capability c and deletes any children it has. If c is not
// an original capability, this has no effect.
message_info_t
revoke_cap (struct cte *c)
{
  while (is_child_cap (c->next, c))
    delete_cap (c->next);

  return msg_ok (0);
}
