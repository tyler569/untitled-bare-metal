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
  max_error_code,
};

extern const char *const error_strings[];

enum cap_type : unsigned char
{
  cap_null,
  cap_untyped,
  cap_endpoint,
  cap_cnode,
  cap_vspace,
  cap_tcb,

  cap_pml4,
  cap_pdpt,
  cap_pd,
  cap_pt,

  cap_frame,

  max_cap_type,
};

extern const char *const cap_type_strings[];

enum object_method_bases
{
  null_base = cap_null * 0x1000,
  tcb_base = cap_tcb * 0x1000,
  cnode_base = cap_cnode * 0x1000,
  frame_base = cap_frame * 0x1000,
  pml4_base = cap_pml4 * 0x1000,
  pdpt_base = cap_pdpt * 0x1000,
  pd_base = cap_pd * 0x1000,
  pt_base = cap_pt * 0x1000,
  untyped_base = cap_untyped * 0x1000,
  endpoint_base = cap_endpoint * 0x1000,
};

#include "sys/obj/cnode.h"
#include "sys/obj/tcb.h"
#include "sys/obj/untyped.h"
