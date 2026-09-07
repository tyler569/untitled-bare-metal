#pragma once
#include "limine.h"
#include "sys/types.h"

constexpr uint8_t INIT_CNODE_SIZE_BITS = 12;

enum boot_capabilities
{
  INIT_CAP_NULL,
  INIT_CAP_INIT_TCB,
  INIT_CAP_ROOT_CNODE,
  INIT_CAP_INIT_VSPACE,
  INIT_CAP_IO_PORT_CONTROL,
  INIT_CAP_IRQ_CONTROL,

  INIT_CAP_FIRST_UNTYPED,
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

  struct limine_framebuffer framebuffer_info;

  size_t n_untypeds;
  struct untyped_desc untypeds[];
};
