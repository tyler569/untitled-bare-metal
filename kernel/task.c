#include "sys/task.h"
#include "sys/slab.h"

LIST_HEAD (tasks);
struct slab_cache task_cache;

void
init_tasks ()
{
  init_list (&tasks);
  init_slab_cache (&task_cache, sizeof (struct task));
}

struct task *
task_create ()
{
  struct task *t = slab_alloc (&task_cache);

  init_list (&t->tasks);
  init_list (&t->children);

  append_to_list (&t->tasks, &tasks);

  return t;
}