#pragma once

#include "assert.h"
#include "kern/cap.h"

static inline struct cap *
cap_for (struct cap *root_cnode, word_t index, word_t depth)
{
  assert (depth == 64);
  struct cap *slots = cap_ptr (root_cnode);
  assert (cap_size (root_cnode) > index);
  return slots + index;
}
