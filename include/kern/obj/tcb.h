#pragma once

#include "assert.h"
#include "elf.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/per_cpu.h"
#include "list.h"
#include "spinlock.h"
#include "sys/ipc.h"
#include "sys/types.h"

#define MAX_PRIORITY 254

enum tcb_state
{
  TASK_STATE_RUNNABLE,
  TASK_STATE_RUNNING,
  TASK_STATE_DEAD,
  TASK_STATE_SENDING,
  TASK_STATE_RECEIVING,
  TASK_STATE_CALLING,
  TASK_STATE_SUSPENDED,
};

struct tcb
{
  enum tcb_state state;

  struct list_head runnable_tcbs;
  struct list_head send_receive_node;

  struct ipc_buffer *ipc_buffer;
  uintptr_t endpoint_badge;

  bool expects_reply;
  struct tcb *reply_to;

  uintptr_t tls_base;

  cte_t cspace_root;
  cte_t vspace_root;
  cte_t ipc_buffer_frame;
  cte_t reply;

  struct notification *bound_notification;

  frame_t saved_state;
  frame_t *current_user_frame;

  bool debug;
  char name[16];
};

void init_tcbs (void *init_elf);

struct tcb *create_tcb (struct tcb *);
struct tcb *create_tcb_from_elf_in_this_vm (struct tcb *,
                                            struct elf_ehdr *elf);
void kill_tcb (struct tcb *t);
void destroy_tcb (struct tcb *t);

void switch_tcb (struct tcb *t);
void return_from_kernel_code ();

void save_tcb_state (struct tcb *t);
void make_tcb_runnable (struct tcb *t);
void schedule ();

void send_message (struct tcb *receiver, uintptr_t message);
void receive_message ();

int invoke_tcb_method (cap_t tcb, word_t method);

void print_this_tcb ();

#define this_tcb (this_cpu->current_tcb)
