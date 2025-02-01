#pragma once
#include "sys/types.h"

#define INIT_CNODE_SIZE_BITS 10

enum boot_capabilities
{
  init_cap_null,
  init_cap_init_tcb,
  init_cap_root_cnode,
  init_cap_init_vspace,
  init_cap_io_port_control,
  init_cap_irq_control,

  init_cap_first_untyped,
};

struct cap_range
{
  cptr_t start;
  cptr_t end;
};

struct untyped_desc
{
  word_t base;
  uint8_t size_bits;
  bool is_device;
};

struct boot_info
{
  word_t node_id;

  void *initrd;
  size_t initrd_size;

  struct cap_range untyped_range;
  struct cap_range empty_range;

  size_t n_untypeds;
  struct untyped_desc untypeds[];
};
