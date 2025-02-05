#pragma once

#include "kern/ipc.h"
#include "stdio.h"
#include "sys/syscall.h"
#include "sys/types.h"

struct frame;
typedef struct frame frame_t;

void do_syscall (uintptr_t, uintptr_t, int syscall_number, frame_t *);

// Used substantially in kern/syscall.c and generated syscall dispatch
#define dbg_printf(...)                                                       \
  do                                                                          \
    {                                                                         \
    }                                                                         \
  while (0)
#define err_printf(...) printf (__VA_ARGS__)

MUST_USE
static inline message_info_t
return_ipc (word_t err, word_t registers)
{
  if (err != no_error && err < max_error_code)
    dbg_printf ("return_ipc: err='%s'\n", error_string (err));
  else if (err != no_error)
    dbg_printf ("return_ipc: err='%lu'\n", err);

  message_info_t info = new_message_info (err, 0, 0, registers);

  set_ipc_info (info);
  return info;
}
