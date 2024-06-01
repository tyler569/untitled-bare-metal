#pragma once

#include "assert.h"
#include "elf.h"
#include "list.h"
#include "spinlock.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/types.h"

#define MAX_PRIORITY 254

enum tcb_methods
{
  tcb_configure = tcb_base,
  tcb_read_registers,
  tcb_write_registers,
  tcb_resume,
  tcb_set_prio,
  tcb_echo,
};
