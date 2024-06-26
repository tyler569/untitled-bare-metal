#pragma once

#include "kern/ipc.h"
#include "stdio.h"
#include "sys/syscall.h"
#include "sys/types.h"

struct frame;
typedef struct frame frame_t;

uintptr_t do_syscall (uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t,
                      uintptr_t, int syscall_number, frame_t *);

static inline void
return_ipc (word_t err, word_t registers)
{
  if (err != no_error && err < max_error_code)
    printf ("return_ipc: err='%s'\n", error_string (err));
  else if (err != no_error)
    printf ("return_ipc: err='%lu'\n", err);

  set_ipc_info (new_message_info (err, 0, 0, registers));
}
