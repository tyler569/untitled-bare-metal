#pragma once

#include "kern/cap.h"
#include "stddef.h"
#include "stdint.h"

struct physical_extent
{
  uintptr_t start;
  size_t len;
};

void init_page_mmap ();

// arch-specific

void get_physical_extents (struct physical_extent *extents, size_t *max);
uintptr_t direct_map_of (uintptr_t addr);
uintptr_t physical_of (uintptr_t addr);

void add_vm_mapping (uintptr_t root, uintptr_t virt, uintptr_t phys,
                     int flags);

uintptr_t alloc_page ();

struct untyped_desc;

void create_init_untyped_caps (cte_t *base, size_t *count,
                               struct untyped_desc *desc);
void create_init_untyped_device_caps (cte_t *base, size_t *count,
                                      struct untyped_desc *desc);
