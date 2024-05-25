#pragma once

#include "assert.h"
#include "list.h"
#include "stddef.h"
#include "stdint.h"

enum physical_extent_flags
{
  PHYSICAL_EXTENT_FREE = 0,
  PHYSICAL_EXTENT_USED = 1 << 0,
};

struct physical_extent
{
  enum physical_extent_flags flags;
  uintptr_t start;
  size_t len;
};

#define PTE_PRESENT (1 << 0)
#define PTE_WRITE (1 << 1)
#define PTE_USER (1 << 2)

void init_page_mmap ();

// arch-specific

void get_physical_extents (struct physical_extent *extents, size_t *max);
uintptr_t direct_map_of (uintptr_t addr);
uintptr_t physical_of (uintptr_t addr);

void add_vm_mapping (uintptr_t root, uintptr_t virt, uintptr_t phys, int flags);

void *kmem_alloc (size_t);
uintptr_t alloc_page ();
