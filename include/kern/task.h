#pragma once

#include "assert.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/per_cpu.h"
#include "sys/obj/task.h"

struct task
{
  enum task_state state;

  struct list_head runnable_tasks;
  struct list_head send_receive_pending_node;

  struct ipc_buffer *ipc_buffer;
  uintptr_t ipc_buffer_user;

  cap_t cspace_root;
  cap_t vspace_root;

  uintptr_t vm_root;

  frame_t saved_state;
  frame_t *current_user_frame;
};

void init_tasks ();

struct task *create_task (struct task *);
struct task *create_task_from_elf_in_this_vm (struct task *,
                                              struct elf_ehdr *elf);
void kill_task (struct task *t);
void destroy_task (struct task *t);

void switch_task (struct task *t);
void save_task_state (struct task *t);
void make_task_runnable (struct task *t);
struct task *pick_next_task ();
void schedule ();

void send_message (struct task *receiver, uintptr_t message);
void receive_message ();

int invoke_task_method ();

#define this_task (this_cpu->current_task)
