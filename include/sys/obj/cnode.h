#pragma once

#include "sys/syscall.h"

enum cnode_methods
{
  cnode_copy = cnode_base,
  cnode_delete,
  cnode_mint,
};
