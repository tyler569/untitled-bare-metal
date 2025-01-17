#pragma once

#define INIT_CNODE_SIZE_BITS 8
#define INIT_CNODE_SIZE (1 << INIT_CNODE_SIZE_BITS)

enum boot_capabilities
{
  init_cap_null,
  init_cap_init_tcb,
  init_cap_root_cnode,
  init_cap_init_vspace,
  init_cap_io_port_control,

  init_cap_first_untyped,
};

struct slot_region
{
  cptr_t start;
  cptr_t end;
};

struct untyped_desc
{
  word_t base;
  uint8_t size_bits;
  uint8_t is_device;
};

struct boot_info
{
  word_t node_id;

  size_t n_untypeds;
  struct untyped_desc untypeds[];
};
