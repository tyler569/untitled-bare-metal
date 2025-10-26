#pragma once

#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "stdio.h"
#include "sys/syscall.h"
#include "sys/types.h"

struct frame;
typedef struct frame frame_t;

void do_syscall (uintptr_t, uintptr_t, int syscall_number);

// Used substantially in kern/syscall.c and generated syscall dispatch
#define dbg_printf(fmt, ...)                                                  \
  do                                                                          \
    if (this_tcb->debug)                                                      \
      printf (fmt __VA_OPT__ (, ) __VA_ARGS__);                               \
  while (0)
#define err_printf(...) printf (__VA_ARGS__)

MUST_USE
static inline error_t
return_ipc (error_t err, word_t registers)
{
  if (err != no_error && err < max_error_code)
    dbg_printf ("return_ipc: err='%s'\n", error_string (err));
  else if (err != no_error)
    dbg_printf ("return_ipc: err='%lu'\n", err);

  message_info_t info = new_message_info (err, 0, 0, registers);
  set_ipc_info (info);

  return err;
}

MUST_USE
static inline error_t
ipc_ok (word_t registers)
{
  return return_ipc (no_error, registers);
}

MUST_USE
static inline error_t
ipc_error (word_t error, word_t registers)
{
  return return_ipc (error, registers);
}

MUST_USE
static inline error_t
ipc_illegal_operation ()
{
  return return_ipc (illegal_operation, 0);
}

MUST_USE
static inline error_t
ipc_range_error (word_t min, word_t max)
{
  set_mr (0, min);
  set_mr (1, max);
  return return_ipc (range_error, 2);
}

MUST_USE
static inline error_t
ipc_truncated_message (word_t expected, word_t provided)
{
  set_mr (0, expected);
  set_mr (1, provided);
  return return_ipc (truncated_message, 2);
}

MUST_USE
static inline error_t
ipc_delete_first ()
{
  return return_ipc (delete_first, 0);
}

MUST_USE
static inline error_t
ipc_invalid_argument (word_t argument_number)
{
  set_mr (0, argument_number);
  return return_ipc (invalid_argument, 1);
}

MUST_USE
static inline error_t
ipc_invalid_root ()
{
  return return_ipc (invalid_root, 0);
}

MUST_USE
static inline error_t
ipc_not_enough_memory (word_t available_memory)
{
  set_mr (0, available_memory);
  return return_ipc (not_enough_memory, 1);
}

MUST_USE
static inline error_t
ipc_failed_lookup (word_t level)
{
  set_mr (0, level);
  return return_ipc (failed_lookup, 1);
}
