#include "kern/cap.h"
#include "assert.h"
#include "kern/mem.h"
#include "kern/obj/tcb.h"
#include "kern/size.h"
#include "sys/types.h"

// Capability Derivation Tree Operations
// Based on the linked-list approach for tracking capability parent-child
// relationships

/**
 * Insert a capability into the derivation tree.
 *
 * @param new The capability to insert
 * @param parent The parent capability (insert after this)
 * @param next The next sibling (insert before this, or NULL if at end)
 *
 * This maintains a doubly-linked list of capabilities derived from the same
 * parent. The parent is the head of the list, and all derived caps follow.
 */
void
cdt_insert (struct cap *new, struct cap *parent, struct cap *next)
{
  if (next)
    cap_set_prev (next, new);
  cap_set_next (parent, new);

  cap_set_next (new, next);
  cap_set_prev (new, parent);
}

/**
 * Remove a capability from the derivation tree.
 *
 * @param cap The capability to remove
 *
 * This unlinks the capability from the doubly-linked list but does not
 * delete the capability itself.
 */
void
cdt_remove (struct cap *cap)
{
  struct cap *prev = cap_prev (cap);
  struct cap *next = cap_next (cap);

  if (prev)
    cap_set_next (prev, next);
  if (next)
    cap_set_prev (next, prev);

  cap_set_next (cap, NULL);
  cap_set_prev (cap, NULL);
}

/**
 * Check if a capability is a child of a parent capability.
 *
 * @param child The potential child capability
 * @param parent The potential parent capability
 * @return true if child is derived from parent
 *
 * The relationship depends on the parent's type:
 * - cap_untyped: children are any typed objects allocated from this untyped
 * - cap_endpoint/cap_notification with badge=0: children are any caps with
 *   matching type (original unbadged endpoint can spawn badged copies)
 * - cap_endpoint/cap_notification with badge: children must have matching
 * badge
 * - Other types: children must have matching type, and parent must be at the
 *   root of a derivation subtree (prev == NULL means original)
 */
bool
is_child (struct cap *child, struct cap *parent)
{
  if (!child)
    return false;

  word_t parent_type = cap_type (parent);

  // Untyped memory: check if child's object pointer is within the untyped
  // region Note: untyped caps store physical addresses, typed objects store
  // HHDM addresses Convert parent to HHDM to compare in same address space
  if (parent_type == cap_untyped)
    {
      uintptr_t child_ptr = (uintptr_t)cap_ptr (child);
      uintptr_t parent_ptr = direct_map_of ((uintptr_t)cap_ptr (parent));
      uintptr_t parent_size = BIT (parent->size_bits);

      return child_ptr >= parent_ptr && child_ptr < parent_ptr + parent_size;
    }

  // Type must match for non-untyped parents
  if (cap_type (child) != parent_type)
    return false;

  // For endpoints and notifications: special badge-based derivation rules
  if (parent_type == cap_endpoint || parent_type == cap_notification)
    {
      // Unbadged original (badge == 0) can spawn badged children
      if (parent->badge == 0)
        {
          // Parent must be at root of derivation tree (original capability)
          return cap_prev (parent) == NULL;
        }
      // Badged parent: child must have matching badge
      return parent->badge == child->badge;
    }

  // For other types: parent must be the original (at root of derivation tree)
  // This prevents copies from being treated as parents
  return cap_prev (parent) == NULL;
}

/**
 * Delete a capability.
 *
 * @param cap The capability to delete
 * @return no_error on success, revoke_first if children exist
 *
 * A capability can only be deleted if it has no children in the derivation
 * tree. If children exist, they must be revoked first.
 *
 * When deleting an original capability to a container object (TCB or CNode),
 * this triggers object destruction, which automatically deletes all contained
 * capabilities first. This matches seL4 semantics where deleting the last
 * (original) capability to an object destroys the object and cleans up its
 * contents.
 *
 * Derived capabilities (is_original == 0) do not trigger object destruction.
 */
error_t
cap_delete (struct cap *cap)
{
  word_t type = cap_type (cap);

  // Check if this capability has children in the derivation tree
  if (is_child (cap_next (cap), cap))
    return revoke_first;

  // Object destruction only happens for original capabilities
  // Derived capabilities just get removed from their slot
  if (cap->is_original)
    {
      // Object destruction for capability-owning objects
      // Automatically delete all contained capabilities before destroying the
      // object
      if (type == cap_tcb)
        {
          struct tcb *tcb = (struct tcb *)cap_ptr (cap);
          error_t err;

          // Delete each contained capability (may trigger recursive
          // destruction)
          if (cap_type (&tcb->cspace_root) != cap_null)
            {
              err = cap_delete (&tcb->cspace_root);
              if (err != no_error)
                return err;
            }

          if (cap_type (&tcb->vspace_root) != cap_null)
            {
              err = cap_delete (&tcb->vspace_root);
              if (err != no_error)
                return err;
            }

          if (cap_type (&tcb->ipc_buffer_frame) != cap_null)
            {
              err = cap_delete (&tcb->ipc_buffer_frame);
              if (err != no_error)
                return err;
            }

          if (cap_type (&tcb->reply) != cap_null)
            {
              err = cap_delete (&tcb->reply);
              if (err != no_error)
                return err;
            }
        }

      if (type == cap_cnode)
        {
          struct cap *slots = (struct cap *)cap_ptr (cap);
          size_t num_slots = cap_size (cap);

          // Delete each contained capability (may trigger recursive
          // destruction)
          for (size_t i = 0; i < num_slots; i++)
            {
              if (cap_type (&slots[i]) != cap_null)
                {
                  error_t err = cap_delete (&slots[i]);
                  if (err != no_error)
                    return err;
                }
            }
        }
    }

  // Remove from derivation tree
  cdt_remove (cap);

  // Clear the capability slot
  cap_null_init (cap);

  return no_error;
}

/**
 * Revoke all derived children in the CDT.
 *
 * @param cap The capability whose CDT children to revoke
 * @return no_error on success
 *
 * This recursively revokes and deletes all capabilities derived from the given
 * capability in the capability derivation tree (CDT), but does NOT delete the
 * capability itself. This matches seL4_CNode_Revoke() semantics.
 *
 * Note: Revoke only affects the CDT derivation tree (capabilities created via
 * Copy/Mint). It does NOT touch contained capabilities in TCB/CNode objects.
 * Those are handled by object destruction when cap_delete() is called on the
 * container.
 *
 * After revoke completes, the capability has no CDT children, so a subsequent
 * delete will succeed.
 */
error_t
cap_revoke (struct cap *cap)
{
  // Recursively revoke and delete all children in derivation tree
  while (is_child (cap_next (cap), cap))
    {
      struct cap *child = cap_next (cap);
      error_t err = cap_revoke (child);
      if (err != no_error)
        return err;

      // Delete the child after revoking its descendants
      // If child is a container, this will trigger object destruction
      err = cap_delete (child);
      if (err != no_error)
        return err;
    }

  return no_error;
}
