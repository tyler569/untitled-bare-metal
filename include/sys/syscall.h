#pragma once

#include "stdint.h"

enum syscall_number
{
  sys_exit,
  sys_debug_write,

  sys_send,
  sys_recv,
  sys_call,
  sys_reply,
  sys_nbsend,
  sys_replyrecv,
  sys_nbrecv,
  sys_yield,
};

enum error_number
{
  no_error,
  invalid_argument,
  invalid_capability,
  illegal_operation,
  range_error,
  alignment_error,
  failed_lookup,
  truncated_message,
  delete_first,
  revoke_first,
  not_enough_memory,
};
