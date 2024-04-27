#include "sys/task.h"
#include "sys/slab.h"

LIST_HEAD (tasks);
struct slab_cache task_cache;

void
init_tasks ()
{
  list_init (&tasks);
  init_slab_cache (&task_cache, sizeof (struct task));
}

struct task *
task_create ()
{
  struct task *t = slab_alloc (&task_cache);

  list_init (&t->tasks);
  list_init (&t->children);

  list_insert_after (&t->tasks, &tasks);

  return t;
}