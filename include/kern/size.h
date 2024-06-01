#pragma once

#include "assert.h"
#include "sys/syscall.h"

enum object_size_bits
{
  endpoint_size_bits = 4,
  cnode_slot_bits = 5,
  tcb_size_bits = 9,

  frame_size_bits = 12,
  vspace_size_bits = 12,
  pdpt_size_bits = 12,
  pd_size_bits = 12,
  pt_size_bits = 12,
};

static inline word_t
object_size (word_t object_type, word_t size_bits)
{
  switch (object_type)
    {
    case cap_null:
      return 0;
    case cap_tcb:
      return BIT (tcb_size_bits);
    case cap_cnode:
      return BIT (size_bits + cnode_slot_bits);
    case cap_x86_64_page:
      return BIT (frame_size_bits);
    case cap_vspace:
      return BIT (vspace_size_bits);
    case cap_x86_64_pdpt:
      return BIT (pdpt_size_bits);
    case cap_x86_64_pd:
      return BIT (pd_size_bits);
    case cap_x86_64_pt:
      return BIT (pt_size_bits);
    case cap_untyped:
      return BIT (size_bits);
    case cap_endpoint:
      return BIT (endpoint_size_bits);
    default:
      assert (0 && "Invalid object type");
    }
}
