#include "sys/syscall.h"

const char *const error_strings[] = {
  [no_error] = "no error",
  [invalid_argument] = "invalid argument",
  [invalid_capability] = "invalid capability",
  [illegal_operation] = "illegal operation",
  [range_error] = "range error",
  [alignment_error] = "alignment error",
  [failed_lookup] = "failed lookup",
  [truncated_message] = "truncated message",
  [delete_first] = "delete first",
  [revoke_first] = "revoke first",
  [not_enough_memory] = "not enough memory",
};

const char *const cap_type_strings[] = {
  [cap_null] = "null",       [cap_tcb] = "tcb",
  [cap_cnode] = "cnode",     [cap_frame] = "frame",
  [cap_pml4] = "pml4",       [cap_pdpt] = "pdpt",
  [cap_pd] = "pd",           [cap_pt] = "pt",
  [cap_untyped] = "untyped", [cap_endpoint] = "endpoint",
  [cap_vspace] = "vspace",
};
