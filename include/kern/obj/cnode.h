#pragma once

#include "kern/cap.h"
#include "sys/cdefs.h"
#include "sys/obj/cnode.h"

static inline cte_t *
cte_for (cte_t *root_cnode, word_t index, word_t depth)
{
  assert (depth == 64);
  cte_t *cte = cap_ptr (root_cnode);
  assert (cap_size (root_cnode) > index);
  return cte + index;
}
