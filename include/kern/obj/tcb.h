#pragma once

#include "assert.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/per_cpu.h"
#include "sys/obj/tcb.h"

enum tcb_state
{
  TASK_STATE_RUNNABLE,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
  TASK_STATE_ZOMBIE,
  TASK_STATE_DEAD,
  TASK_STATE_SENDING,
  TASK_STATE_RECEIVING,
};

struct tcb
{
  enum tcb_state state;

  struct list_head runnable_tcbs;
  struct list_head send_receive_pending_node;

  struct ipc_buffer *ipc_buffer;
  uintptr_t ipc_buffer_user;

  cap_t cspace_root;
  cap_t vspace_root;

  uintptr_t vm_root;

  frame_t saved_state;
  frame_t *current_user_frame;
};

void init_tcbs (void *init_elf);

struct tcb *create_tcb (struct tcb *);
struct tcb *create_tcb_from_elf_in_this_vm (struct tcb *,
                                            struct elf_ehdr *elf);
void kill_tcb (struct tcb *t);
void destroy_tcb (struct tcb *t);

void switch_tcb (struct tcb *t);
void save_tcb_state (struct tcb *t);
void make_tcb_runnable (struct tcb *t);
struct tcb *pick_next_tcb ();
void schedule ();

void send_message (struct tcb *receiver, uintptr_t message);
void receive_message ();

int invoke_tcb_method (cap_t tcb, word_t method);

#define this_tcb (this_cpu->current_tcb)
