#include "assert.h"
#include "kern/cap.h"
#include "kern/mem.h"
#include "sys/syscall.h"

void
insert (struct cte *new, struct cte *src, struct cte *next)
{
  if (next)
    next->prev = new;
  src->next = new;

  new->next = next;
  new->prev = src;
}

void
remove (struct cte *del)
{
  if (del->prev)
    del->prev->next = del->next;
  if (del->next)
    del->next->prev = del->prev;
  del->next = nullptr;
  del->prev = nullptr;
}

error_t
copy (struct cte *dest, struct cte *src, cap_rights_t rights_mask)
{
  if (cap_type (dest) != cap_null)
    return delete_first;

  if (cap_type (src) == cap_untyped && src->cap.badge)
    return revoke_first;

  dest->cap = src->cap;

  if (cap_type (src) != cap_untyped)
    dest->cap.is_original = false;

  cap_set_rights (dest, cap_rights (src) & rights_mask);

  insert (dest, src, src->next);

  return no_error;
}

error_t
mint (struct cte *dest, struct cte *src, unsigned long badge,
      cap_rights_t rights_mask)
{
  if (cap_type (dest) != cap_null)
    return delete_first;

  if (cap_type (src) != cap_endpoint && cap_type (src) != cap_notification)
    return illegal_operation;

  if (src->cap.badge != 0)
    return illegal_operation;

  dest->cap = src->cap;
  dest->cap.badge = badge;

  if (cap_type (src) == cap_endpoint)
    dest->cap.is_original = true;

  cap_set_rights (dest, cap_rights (src) & rights_mask);

  insert (dest, src, src->next);

  return no_error;
}

bool
is_child (struct cte *c, struct cte *parent)
{
  assert (parent);

  if (!c)
    return false;
  if (parent->cap.type == cap_untyped)
    {
      uintptr_t parent_base = (uintptr_t)cap_ptr (parent);
      uintptr_t parent_len = cap_size (parent);
      uintptr_t parent_end = parent_base + parent_len;
      uintptr_t child_base = physical_of ((uintptr_t)cap_ptr (c));

      return child_base >= parent_base && child_base < parent_end;
    }
  if (c->cap.type != parent->cap.type)
    return false;
  if (!parent->cap.is_original)
    return false;
  if (parent->cap.type == cap_endpoint && parent->cap.badge == 0)
    return true;
  if (parent->cap.type == cap_endpoint)
    return parent->cap.badge == c->cap.badge;
  return true;
}

error_t
delete (struct cte *c)
{
  revoke (c);

  remove (c);

  if (c->cap.is_original)
    {
      // TODO: deletion operation on kernel object; cascading
    }

  c->cap = cap_null_new ();

  return no_error;
}

error_t
revoke (struct cte *c)
{
  while (is_child (c->next, c))
    {
      revoke (c->next);
      delete (c->next);
    }

  // leaves c with no children

  return no_error;
}
