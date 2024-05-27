#include "kern/obj/tcb.h"
#include "assert.h"
#include "kern/ipc.h"
#include "kern/mem.h"
#include "kern/per_cpu.h"
#include "string.h"
#include "sys/syscall.h"

// struct list_head runnable_tcbs[MAX_PRIORITY];
LIST_HEAD (runnable_tcbs);
spin_lock_t runnable_tcbs_lock;

void
init_tcbs ()
{
}

struct tcb *
create_tcb (struct tcb *t)
{
  memset (t, 0, sizeof (struct tcb));

  init_list (&t->runnable_tcbs);
  init_list (&t->send_receive_pending_node);

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

  uintptr_t stack = 0x7ffffff00000;

  add_vm_mapping (t->vm_root, stack, alloc_page (),
                  PTE_PRESENT | PTE_USER | PTE_WRITE);

  new_user_frame (&t->saved_state, elf_entry (elf), stack + PAGE_SIZE);

  uintptr_t ipc_buffer = 0x7fffffe00000;
  uintptr_t ipc_buffer_page = alloc_page ();

  add_vm_mapping (t->vm_root, ipc_buffer, ipc_buffer_page,
                  PTE_PRESENT | PTE_USER | PTE_WRITE);
  set_frame_arg (&t->saved_state, 0, ipc_buffer);

  t->ipc_buffer_user = ipc_buffer;
  t->ipc_buffer = (struct ipc_buffer *)direct_map_of (ipc_buffer_page);

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

int
invoke_tcb_method ()
{
  cptr_t target_cptr = get_extra_cap (0);
  cap_t tcb;
  exception_t status
      = lookup_cap (this_tcb->cspace_root, target_cptr, 64, &tcb);

  if (status != no_error)
    {
      return_ipc_error (status, 0);
      return 1;
    }

  struct tcb *t = cap_ptr (tcb);

  assert (tcb.type == CAP_TCB);

  switch (get_ipc_label ())
    {
    case tcb_resume:
      make_tcb_runnable (t);
      break;
    default:
      return_ipc_error (invalid_argument, 0);
      return 1;
    };

  return 0;
}

void
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
