#include "sys/task.h"
#include "assert.h"
#include "string.h"
#include "sys/mem.h"
#include "sys/slab.h"

LIST_HEAD (tasks);
struct slab_cache task_cache;

LIST_HEAD (runnable_tasks);
spin_lock_t runnable_tasks_lock;

void
init_tasks ()
{
  init_list (&tasks);
  init_slab_cache (&task_cache, sizeof (struct task));
}

struct task *
create_task ()
{
  struct task *t = slab_alloc (&task_cache);
  memset (t, 0, sizeof (struct task));

  init_list (&t->tasks);
  init_list (&t->runnable_tasks);
  init_list (&t->send_receive_pending);
  init_list (&t->send_receive_pending_node);

  append_to_list (&t->tasks, &tasks);

  return t;
}

struct task *
create_task_in_this_vm (uintptr_t rip, uintptr_t rsp)
{
  struct task *t = create_task ();

  t->vm_root = get_vm_root ();
  t->saved_state = new_user_frame (rip, rsp);

  return t;
}

struct task *
create_task_from_elf_in_this_vm (struct elf_ehdr *elf)
{
  struct task *t = create_task ();

  t->elf = elf;
  t->vm_root = get_vm_root ();

  elf_load (elf);

  uintptr_t stack = 0x7ffffff00000;

  add_vm_mapping (t->vm_root, stack, alloc_page (),
                  PTE_PRESENT | PTE_USER | PTE_WRITE);

  t->saved_state = new_user_frame (elf_entry (elf), stack + PAGE_SIZE);

  return t;
}

struct task *
create_task_from_elf_in_new_vm (struct elf_ehdr *elf)
{
  struct task *t = create_task ();

  t->elf = elf;
  t->vm_root = new_page_table ();
  set_vm_root (t->vm_root);

  elf_load (elf);

  uintptr_t stack = 0x7ffffff00000;

  add_vm_mapping (t->vm_root, stack, alloc_page (),
                  PTE_PRESENT | PTE_USER | PTE_WRITE);

  t->saved_state = new_user_frame (elf_entry (elf), stack + PAGE_SIZE);

  return t;
}

struct task *
create_task_from_syscall (bool, uintptr_t arg)
{
  struct task *t
      = create_task_from_elf_in_new_vm (this_cpu->current_task->elf);
  set_frame_arg (t->saved_state, 0, arg);
  set_vm_root (this_cpu->current_task->vm_root);
  return t;
}

void
destroy_task (struct task *t)
{
  assert (is_list_empty (&t->runnable_tasks));
  assert (t->state == TASK_STATE_DEAD);

  remove_from_list (&t->tasks);
  slab_free (&task_cache, t);
}

void
save_task_state (struct task *t)
{
  copy_frame (t->saved_state, t->current_user_frame);
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
  struct task *current = this_cpu->current_task;

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
  jump_to_userland_frame (t->saved_state);
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
  struct task *current = this_cpu->current_task;
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
