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
  init_list (&t->children);
  init_list (&t->siblings);
  init_list (&t->runnable_tasks);

  append_to_list (&t->tasks, &tasks);

  return t;
}

void
destroy_task (struct task *t)
{
  assert (is_list_empty (&t->children));
  remove_from_list (&t->tasks);
  slab_free (&task_cache, t);
}

struct task *
create_first_task (struct elf_ehdr *elf)
{
  struct task *t = create_task ();

  t->elf = elf;
  t->vm_root = get_vm_root ();

  elf_load (elf);

  uintptr_t stack = 0x7ffffff00000;

  add_vm_mapping (t->vm_root, stack, alloc_page (),
                  PTE_PRESENT | PTE_USER | PTE_WRITE);

  t->frame = new_user_frame (elf_entry (elf), stack + PAGE_SIZE);

  return t;
}

void
switch_task (struct task *t)
{
  t->state = TASK_STATE_RUNNING;

  this_cpu->current_task = t;
  set_vm_root (t->vm_root);
  jump_to_userland_frame (t->frame);
}

void
kill_task (struct task *t)
{
  t->state = TASK_STATE_DEAD;
}

void
make_task_runnable (struct task *t)
{
  assert (t->state != TASK_STATE_DEAD);
  assert (t->state != TASK_STATE_RUNNING);

  t->state = TASK_STATE_RUNNABLE;

  spin_lock (&runnable_tasks_lock);
  append_to_list (&t->runnable_tasks, &runnable_tasks);
  spin_unlock (&runnable_tasks_lock);
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

  t->state = TASK_STATE_RUNNING;

  return t;
}

void
schedule ()
{
  struct task *current = this_cpu->current_task;
  struct task *next = pick_next_task ();

  if (next == nullptr && current && current->state == TASK_STATE_RUNNING)
    return;

  if (next == nullptr)
    while (true)
      halt_until_interrupt ();

  if (current)
    make_task_runnable (current);
  switch_task (next);
}
