#pragma once

#include "assert.h"
#include "elf.h"
#include "list.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/spinlock.h"
#include "sys/types.h"

#define MAX_PRIORITY 254

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

enum task_methods
{
  TCB_CONFIGURE = tcb_methods,
  TCB_RESUME,
  TCB_SET_PRIORITY,
};
