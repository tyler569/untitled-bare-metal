#pragma once

#include "assert.h"
#include "elf.h"
#include "list.h"
#include "spinlock.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/types.h"

#define MAX_PRIORITY 254

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

enum tcb_methods
{
  tcb_configure = tcb_base,
  tcb_resume,
  tcb_set_prio,
};
