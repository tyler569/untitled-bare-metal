#pragma once

#include "elf.h"
#include "list.h"
#include "sys/arch.h"
#include "sys/cdefs.h"
#include "sys/spinlock.h"

enum task_state
{
  TASK_STATE_RUNNABLE,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
  TASK_STATE_ZOMBIE,
  TASK_STATE_DEAD,
  TASK_STATE_SENDING,
  TASK_STATE_RECEIVING,
};

struct task
{
  enum task_state state;

  struct list_head tasks;
  struct list_head runnable_tasks;

  spin_lock_t message_lock;
  struct list_head send_receive_pending;
  struct list_head send_receive_pending_node;

  uintptr_t sending_message;

  struct elf_ehdr *elf;
  uintptr_t vm_root;

  frame_t *saved_state;
  frame_t *current_user_frame;
};

void init_tasks ();

struct task *create_task ();
struct task *create_task_from_elf_in_this_vm (struct elf_ehdr *elf);
struct task *create_task_from_elf_in_new_vm (struct elf_ehdr *elf);
struct task *create_task_in_this_vm (uintptr_t rip, uintptr_t rsp);
struct task *create_task_from_syscall (bool in_this_vm, uintptr_t arg);
void kill_task (struct task *t);
void destroy_task (struct task *t);

void switch_task (struct task *t);
void save_task_state (struct task *t);
void make_task_runnable (struct task *t);
struct task *pick_next_task ();
void schedule ();

void send_message (struct task *receiver, uintptr_t message);
void receive_message ();
