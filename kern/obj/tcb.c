#include "kern/obj/tcb.h"
#include "assert.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/elf.h"
#include "kern/mem.h"
#include "kern/methods.h"
#include "kern/obj/notification.h"
#include "kern/per_cpu.h"
#include "kern/size.h"
#include "kern/syscall.h"
#include "stdio.h"
#include "string.h"
#include "sys/syscall.h"

static_assert (sizeof (struct tcb) <= BIT (tcb_size_bits),
               "tcb size is too large");

// struct list_head runnable_tcbs[MAX_PRIORITY];
LIST_HEAD (runnable_tcbs);
spin_lock_t runnable_tcbs_lock;

struct tcb *
create_tcb (struct tcb *t)
{
  memset (t, 0, sizeof (struct tcb));

  return t;
}

struct tcb *
create_tcb_from_elf_in_this_vm (struct tcb *t, struct elf_ehdr *elf)
{
  create_tcb (t);

  elf_load (elf);

  new_user_frame (&t->saved_state, elf->entry, 0);

  return t;
}

void
destroy_tcb (struct tcb *t)
{
  assert (is_list_empty (&t->runnable_tcbs));
  assert (t->state == TASK_STATE_DEAD);
}

void
save_tcb_state (struct tcb *t)
{
  assert (t->current_user_frame);

  copy_frame (&t->saved_state, t->current_user_frame);
}

void
make_tcb_runnable (struct tcb *t)
{
  assert (t->state != TASK_STATE_DEAD);

  t->state = TASK_STATE_RUNNABLE;

  spin_lock (&runnable_tcbs_lock);
  append_to_list (&t->runnable_tcbs, &runnable_tcbs);
  spin_unlock (&runnable_tcbs_lock);
}

void
suspend_tcb (struct tcb *t)
{
  t->state = TASK_STATE_SUSPENDED;

  spin_lock (&runnable_tcbs_lock);
  remove_from_list (&t->runnable_tcbs);
  spin_unlock (&runnable_tcbs_lock);
}

error_t
tcb_resume (cte_t *cap)
{
  struct tcb *tcb = cap_ptr (cap);
  make_tcb_runnable (tcb);
  return no_error;
}

error_t
tcb_suspend (cte_t *cap)
{
  struct tcb *tcb = cap_ptr (cap);
  suspend_tcb (tcb);
  return no_error;
}

error_t
tcb_read_registers (cte_t *cap, bool suspend_source, word_t arch_flags,
                    word_t count, frame_t *regs)
{
  (void)suspend_source;
  (void)arch_flags;
  (void)count;

  struct tcb *tcb = cap_ptr (cap);
  copy_frame (regs, &tcb->saved_state);
  return no_error;
}

error_t
tcb_write_registers (cte_t *cap, bool resume_target, word_t arch_flags,
                     word_t count, frame_t *regs)
{
  (void)resume_target;
  (void)arch_flags;
  (void)count;

  struct tcb *tcb = cap_ptr (cap);
  copy_frame (&tcb->saved_state, regs);
  return no_error;
}

uintptr_t
tcb_vm_root (struct tcb *t)
{
  void *vm_root_page = cap_ptr (&t->vspace_root);
  return physical_of ((uintptr_t)vm_root_page);
}

error_t
tcb_configure (cte_t *slot, word_t fault_ep, cte_t *cspace_root,
               word_t cspace_root_data, cte_t *vspace_root,
               word_t vspace_root_data, word_t buffer, cte_t *buffer_frame)
{
  (void)fault_ep;
  (void)cspace_root_data;
  (void)vspace_root_data;
  (void)buffer;

  struct tcb *tcb = cap_ptr (slot);

  copy_cap (&tcb->cspace_root, cspace_root, cap_rights_all);
  copy_cap (&tcb->vspace_root, vspace_root, cap_rights_all);
  copy_cap (&tcb->ipc_buffer_frame, buffer_frame, cap_rights_all);

  tcb->ipc_buffer = cap_ptr (buffer_frame);

  return no_error;
}

error_t
tcb_bind_notification (cte_t *cap, cte_t *notification)
{
  struct tcb *tcb = cap_ptr (cap);
  if (cap_type (notification) != cap_notification)
    return illegal_operation;

  struct notification *n = cap_ptr (notification);
  if (n->bound_tcb)
    return illegal_operation;

  tcb->bound_notification = n;
  n->bound_tcb = tcb;

  return no_error;
}

error_t
tcb_set_tls_base (cte_t *cap, word_t tls_base)
{
  struct tcb *tcb = cap_ptr (cap);
  tcb->tls_base = tls_base;
  if (tcb == this_tcb)
    set_tls_base (tls_base);
  return no_error;
}

error_t
tcb_set_debug (cte_t *cap, word_t flags)
{
  struct tcb *tcb = cap_ptr (cap);
  tcb->debug = flags != 0;
  return no_error;
}

error_t
tcb_set_name (cte_t *cap, char *name, word_t len)
{
  struct tcb *tcb = cap_ptr (cap);
  strncpy (tcb->name, name, len);
  return no_error;
}

void
print_this_tcb ()
{
  if (this_tcb)
    if (this_tcb->name[0])
      printf ("this_tcb: %p (known as %s)\n", this_tcb, this_tcb->name);
    else
      printf ("this_tcb: %p\n", this_tcb);
  else
    printf ("no tcb\n");
}

void
switch_tcb (struct tcb *t)
{
  assert_eq (t->state, TASK_STATE_RUNNABLE);
  this_cpu->return_to_tcb = t;
}

void
switch_tcb_actual (struct tcb *t)
{
  struct tcb *current = this_tcb;
  assert (current != t);

  // Save the current tcb unless it is dead.
  if (current && current->state != TASK_STATE_DEAD)
    save_tcb_state (current);

  // Make the current tcb runnable if it still wants to run.
  if (current && current->state == TASK_STATE_RUNNING)
    make_tcb_runnable (current);

  assert_eq (t->state, TASK_STATE_RUNNABLE);

  this_cpu->current_tcb = t;
  set_vm_root (tcb_vm_root (t));
  set_tls_base (t->tls_base);
  t->state = TASK_STATE_RUNNING;
  ensure_frame_valid_for_usermode (&t->saved_state);

  jump_to_userland_frame (&t->saved_state);
}

void
return_from_kernel_code ()
{
  struct tcb *current = this_tcb;

  if (this_cpu->return_to_tcb == this_tcb)
    return;
  else if (this_cpu->return_to_tcb)
    switch_tcb_actual (this_cpu->return_to_tcb);
  else
    {
      if (current && current->state != TASK_STATE_DEAD)
        save_tcb_state (current);
      halt_forever_interrupts_enabled ();
    }
}

void
kill_tcb (struct tcb *t)
{
  t->state = TASK_STATE_DEAD;
  destroy_tcb (t);
}

struct tcb *
pick_next_tcb ()
{
  spin_lock (&runnable_tcbs_lock);
  if (is_list_empty (&runnable_tcbs))
    {
      spin_unlock (&runnable_tcbs_lock);
      return nullptr;
    }

  struct tcb *t = CONTAINER_OF (pop_from_list (&runnable_tcbs), struct tcb,
                                runnable_tcbs);
  spin_unlock (&runnable_tcbs_lock);

  return t;
}

void
schedule ()
{
  const struct tcb *current = this_tcb;
  struct tcb *next = pick_next_tcb ();

  // There is no tcb who wants to run, but the current one
  // wants to continue.
  if (!next && current && current->state == TASK_STATE_RUNNING)
    return;

  // There is no tcb who wants to run at all.
  if (!next)
    this_cpu->return_to_tcb = nullptr;
  else
    switch_tcb (next);
}
