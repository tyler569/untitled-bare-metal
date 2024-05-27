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

enum object_type
{
  null_object,
  tcb_object,
  cnode_object,
  frame_object,
  pml4_object,
  pdpt_object,
  pd_object,
  pt_object,
  untyped_object,
  endpoint_object,
};

enum object_method_bases
{
  null_base = null_object * 0x1000,
  tcb_base = tcb_object * 0x1000,
  cnode_base = cnode_object * 0x1000,
  frame_base = frame_object * 0x1000,
  pml4_base = pml4_object * 0x1000,
  pdpt_base = pdpt_object * 0x1000,
  pd_base = pd_object * 0x1000,
  pt_base = pt_object * 0x1000,
  untyped_base = untyped_object * 0x1000,
  endpoint_base = endpoint_object * 0x1000,
};
