#include "sys/task.h"
#include "assert.h"
#include "string.h"
#include "sys/mem.h"
#include "sys/per_cpu.h"
#include "sys/slab.h"

LIST_HEAD (runnable_tasks);
spin_lock_t runnable_tasks_lock;

void
init_tasks ()
{
}

struct task *
create_task (struct task *t)
{
  memset (t, 0, sizeof (struct task));

  init_list (&t->runnable_tasks);
  init_list (&t->send_receive_pending_node);

  return t;
}

struct task *
create_task_in_this_vm (struct task *t, uintptr_t rip, uintptr_t rsp)
{
  create_task (t);

  t->vm_root = get_vm_root ();
  new_user_frame (&t->saved_state, rip, rsp);

  return t;
}

struct task *
create_task_from_elf_in_this_vm (struct task *t, struct elf_ehdr *elf)
{
  create_task (t);

  t->vm_root = get_vm_root ();

  elf_load (elf);

  uintptr_t stack = 0x7ffffff00000;

  add_vm_mapping (t->vm_root, stack, alloc_page (),
                  PTE_PRESENT | PTE_USER | PTE_WRITE);

  new_user_frame (&t->saved_state, elf_entry (elf), stack + PAGE_SIZE);

  return t;
}

void
destroy_task (struct task *t)
{
  assert (is_list_empty (&t->runnable_tasks));
  assert (t->state == TASK_STATE_DEAD);
}

void
save_task_state (struct task *t)
{
  copy_frame (&t->saved_state, t->current_user_frame);
}

void
make_task_runnable (struct task *t)
{
  assert (t->state != TASK_STATE_DEAD);

  t->state = TASK_STATE_RUNNABLE;

  spin_lock (&runnable_tasks_lock);
  append_to_list (&t->runnable_tasks, &runnable_tasks);
  spin_unlock (&runnable_tasks_lock);
}

void
switch_task (struct task *t)
{
  struct task *current = this_task;

  // Save the current task unless it is dead.
  if (current && current->state != TASK_STATE_DEAD)
    save_task_state (current);

  // Make the current task runnable if it still wants to run.
  if (current && current->state == TASK_STATE_RUNNING)
    make_task_runnable (current);

  assert_eq (t->state, TASK_STATE_RUNNABLE);

  t->state = TASK_STATE_RUNNING;

  this_cpu->current_task = t;
  set_vm_root (t->vm_root);
  jump_to_userland_frame (&t->saved_state);
}

void
kill_task (struct task *t)
{
  t->state = TASK_STATE_DEAD;
  destroy_task (t);
}

struct task *
pick_next_task ()
{
  struct task *t;

  spin_lock (&runnable_tasks_lock);
  if (is_list_empty (&runnable_tasks))
    {
      spin_unlock (&runnable_tasks_lock);
      return nullptr;
    }
  t = CONTAINER_OF (pop_from_list (&runnable_tasks), struct task,
                    runnable_tasks);
  spin_unlock (&runnable_tasks_lock);

  return t;
}

void
schedule ()
{
  struct task *current = this_task;
  struct task *next = pick_next_task ();

  // There is no task who wants to run, but the current one
  // wants to continue.
  if (!next && current && current->state == TASK_STATE_RUNNING)
    return;

  // There is no task who wants to run at all.
  if (!next)
    halt_forever_interrupts_enabled ();

  switch_task (next);
}
