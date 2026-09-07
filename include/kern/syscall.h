#pragma once

#include "kern/ipc.h"
#include "kern/obj/tcb.h"
#include "stdio.h"
#include "sys/syscall.h"
#include "sys/types.h"

struct frame;
typedef struct frame frame_t;

void do_syscall (uintptr_t, uintptr_t, enum syscall_number);

// Used substantially in kern/syscall.c and generated syscall dispatch
#define dbg_printf(...)                                                       \
  do                                                                          \
    if (this_tcb->debug)                                                      \
      printf (__VA_ARGS__);                                                   \
  while (0)
#define err_printf(...) printf (__VA_ARGS__)

// TRY macro for error propagation
#define TRY(expr)                                                             \
  do                                                                          \
    {                                                                         \
      message_info_t __tag = (expr);                                          \
      if (__tag.label != NO_ERROR)                                            \
        return __tag;                                                         \
    }                                                                         \
  while (0)

// Helper to create success message_info_t
static inline message_info_t
msg_ok (word_t registers)
{
  return new_message_info (NO_ERROR, 0, 0, registers);
}

// Helper to create error message_info_t
static inline message_info_t
msg_err (word_t label, word_t registers)
{
  if (label != NO_ERROR && label < MAX_ERROR_CODE)
    dbg_printf ("msg_err: err='%s'\n", error_string (label));
  else if (label != NO_ERROR)
    dbg_printf ("msg_err: err='%lu'\n", label);

  return new_message_info (label, 0, 0, registers);
}

MUST_USE
static inline message_info_t
msg_range_error (word_t min, word_t max)
{
  set_mr (0, min);
  set_mr (1, max);
  return msg_err (RANGE_ERROR, 2);
}

MUST_USE
static inline message_info_t
msg_truncated_message (word_t expected, word_t provided)
{
  set_mr (0, expected);
  set_mr (1, provided);
  return msg_err (TRUNCATED_MESSAGE, 2);
}

MUST_USE
static inline message_info_t
msg_delete_first ()
{
  return msg_err (DELETE_FIRST, 0);
}

MUST_USE
static inline message_info_t
msg_revoke_first ()
{
  return msg_err (REVOKE_FIRST, 0);
}

MUST_USE
static inline message_info_t
msg_invalid_argument (word_t argument_number)
{
  set_mr (0, argument_number);
  return msg_err (INVALID_ARGUMENT, 1);
}

MUST_USE
static inline message_info_t
msg_invalid_root ()
{
  return msg_err (INVALID_ROOT, 0);
}

MUST_USE
static inline message_info_t
msg_not_enough_memory (word_t available_memory)
{
  set_mr (0, available_memory);
  return msg_err (NOT_ENOUGH_MEMORY, 1);
}

MUST_USE
static inline message_info_t
msg_failed_lookup (word_t level)
{
  set_mr (0, level);
  return msg_err (FAILED_LOOKUP, 1);
}

MUST_USE
static inline message_info_t
msg_illegal_operation ()
{
  return msg_err (ILLEGAL_OPERATION, 0);
}

// This is used internally to represent the condition that "this ipc return
// will never be actually delivered to the user (for example out of a kernel
// function that calls schedule () ). This is never a valid label for any
// response out of the kernel (our contract is that label is always 0
// (no_error) for all successful operations and data is in MRs)
MUST_USE
static inline message_info_t
msg_noreturn ()
{
  return message_info_from_word (~0UL);
}

static inline bool
msg_is_noreturn (message_info_t msg)
{
  return message_info_to_word (msg) == ~0UL;
}
