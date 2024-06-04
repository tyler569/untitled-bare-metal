#include "kern/obj/tcb.h"
#include "assert.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/mem.h"
#include "kern/methods.h"
#include "kern/per_cpu.h"
#include "kern/size.h"
#include "kern/syscall.h"
#include "stdio.h"
#include "string.h"
#include "sys/bootinfo.h"
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
create_tcb_in_this_vm (struct tcb *t, uintptr_t rip, uintptr_t rsp)
{
  create_tcb (t);

  t->vm_root = get_vm_root ();
  new_user_frame (&t->saved_state, rip, rsp);

  return t;
}

struct tcb *
create_tcb_from_elf_in_this_vm (struct tcb *t, struct elf_ehdr *elf)
{
  create_tcb (t);

  t->vm_root = get_vm_root ();

  elf_load (elf);

  new_user_frame (&t->saved_state, elf_entry (elf), 0);

  return t;
}

void
configure_tcb (struct tcb *t, cap_t cspace_root, cap_t vspace_root,
               uintptr_t ipc_buffer)
{
  t->cspace_root = cspace_root;
  t->vspace_root = vspace_root;
  t->ipc_buffer = (struct ipc_buffer *)direct_map_of (ipc_buffer);
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

error_t
tcb_resume (cte_t *cap)
{
  struct tcb *tcb = cap_ptr (cap);
  printf ("tcb_resume: %p\n", tcb);
  make_tcb_runnable (tcb);
  return no_error;
}

error_t
tcb_echo (cte_t *cap)
{
  (void)cap;

  printf ("echo\n");
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

error_t
tcb_configure (cte_t *slot, word_t fault_ep, cte_t *cspace_root,
               word_t cspace_root_data, cte_t *vspace_root,
               word_t vspace_root_data, word_t buffer, cte_t *buffer_frame)
{
  (void)fault_ep;
  (void)cspace_root_data;
  (void)vspace_root_data;
  (void)buffer;
  (void)buffer_frame;

  struct tcb *tcb = cap_ptr (slot);

  tcb->cspace_root = cspace_root->cap;
  tcb->vspace_root = vspace_root->cap;
  tcb->vm_root = get_vm_root (); // TODO HACK
  tcb->ipc_buffer = (struct ipc_buffer *)buffer;
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

[[noreturn]] void
switch_tcb (struct tcb *t)
{
  struct tcb *current = this_tcb;

  // Save the current tcb unless it is dead.
  if (current && current->state != TASK_STATE_DEAD)
    save_tcb_state (current);

  // Make the current tcb runnable if it still wants to run.
  if (current && current->state == TASK_STATE_RUNNING)
    make_tcb_runnable (current);

  assert_eq (t->state, TASK_STATE_RUNNABLE);

  t->state = TASK_STATE_RUNNING;

  this_cpu->current_tcb = t;
  set_vm_root (t->vm_root);
  set_tls_base (t->tls_base);
  jump_to_userland_frame (&t->saved_state);
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
  struct tcb *t;

  spin_lock (&runnable_tcbs_lock);
  if (is_list_empty (&runnable_tcbs))
    {
      spin_unlock (&runnable_tcbs_lock);
      return nullptr;
    }
  t = CONTAINER_OF (pop_from_list (&runnable_tcbs), struct tcb, runnable_tcbs);
  spin_unlock (&runnable_tcbs_lock);

  return t;
}

void
schedule ()
{
  struct tcb *current = this_tcb;
  struct tcb *next = pick_next_tcb ();

  // There is no tcb who wants to run, but the current one
  // wants to continue.
  if (!next && current && current->state == TASK_STATE_RUNNING)
    return;

  // There is no tcb who wants to run at all.
  if (!next)
    halt_forever_interrupts_enabled ();

  switch_tcb (next);
}
