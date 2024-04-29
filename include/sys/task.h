#pragma once

#include "elf.h"
#include "list.h"
#include "sys/arch.h"
#include "sys/cdefs.h"

#ifdef __x86_64__
#include "arch/x86_64/exports.h"
#endif

enum task_state
{
  TASK_STATE_RUNNABLE,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
  TASK_STATE_ZOMBIE,
  TASK_STATE_DEAD,
};

struct task
{
  enum task_state state;

  struct list_head tasks;
  struct list_head children;
  struct list_head siblings;
  struct list_head runnable_tasks;

  struct elf_ehdr *elf;
  uintptr_t vm_root;

  frame_t *saved_state;
  frame_t *current_interrupt_frame;
};

struct per_cpu
{
  struct per_cpu *self;
  struct arch_per_cpu arch;
  uintptr_t kernel_stack_top;
  struct task *current_task;
  struct list_head list;
  bool printing_backtrace;
};

typedef struct per_cpu per_cpu_t;

void init_tasks ();

struct task *create_task ();
void destroy_task (struct task *t);
struct task *create_first_task (struct elf_ehdr *elf);
void kill_task (struct task *t);

void switch_task (struct task *t);
void make_task_runnable (struct task *t);
struct task *pick_next_task ();
void schedule ();
