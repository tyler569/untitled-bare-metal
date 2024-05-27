#pragma once

#include "assert.h"
#include "sys/syscall.h"

enum object_size_bits
{
  endpoint_size_bits = 4,
  cnode_slot_bits = 5,
  tcb_size_bits = 9,

  frame_size_bits = 12,
  pml4_size_bits = 12,
  pdpt_size_bits = 12,
  pd_size_bits = 12,
  pt_size_bits = 12,
};

static inline word_t
object_size (word_t object_type, word_t size_bits)
{
  switch (object_type)
    {
    case null_object:
      return 0;
    case tcb_object:
      return BIT (tcb_size_bits);
    case cnode_object:
      return BIT (size_bits + cnode_slot_bits);
    case frame_object:
      return BIT (frame_size_bits);
    case pml4_object:
      return BIT (pml4_size_bits);
    case pdpt_object:
      return BIT (pdpt_size_bits);
    case pd_object:
      return BIT (pd_size_bits);
    case pt_object:
      return BIT (pt_size_bits);
    case untyped_object:
      return BIT (size_bits);
    case endpoint_object:
      return BIT (endpoint_size_bits);
    default:
      assert (0 && "Invalid object type");
    }
}
