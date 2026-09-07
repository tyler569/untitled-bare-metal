#pragma once

#include "assert.h"
#include "sys/syscall.h"

enum object_size_bits
{
  ENDPOINT_SIZE_BITS = 4,
  NOTIFICATION_SIZE_BITS = 5,
  CNODE_SLOT_BITS = 5,
  TCB_SIZE_BITS = 9,

  FRAME_SIZE_BITS = 12,
  HUGE_FRAME_SIZE_BITS = 21,

  PML4_SIZE_BITS = 12,
  PDPT_SIZE_BITS = 12,
  PD_SIZE_BITS = 12,
  PT_SIZE_BITS = 12,
};

static inline word_t
object_size (word_t object_type, word_t size_bits)
{
  switch (object_type)
    {
    case CAP_NULL:
      return 0;
    case CAP_TCB:
      return BIT (TCB_SIZE_BITS);
    case CAP_CNODE:
      return BIT (size_bits + CNODE_SLOT_BITS);
    case CAP_ENDPOINT:
      return BIT (ENDPOINT_SIZE_BITS);
    case CAP_NOTIFICATION:
      return BIT (NOTIFICATION_SIZE_BITS);
    case CAP_UNTYPED:
      return BIT (size_bits);
    case CAP_X86_64_PML4:
      return BIT (PML4_SIZE_BITS);
    case CAP_X86_64_PAGE:
      return BIT (FRAME_SIZE_BITS);
    case CAP_X86_64_HUGE_PAGE:
      return BIT (HUGE_FRAME_SIZE_BITS);
    case CAP_X86_64_PDPT:
      return BIT (PDPT_SIZE_BITS);
    case CAP_X86_64_PD:
      return BIT (PD_SIZE_BITS);
    case CAP_X86_64_PT:
      return BIT (PT_SIZE_BITS);
    default:
      assert (0 && "Invalid object type");
    }
}
