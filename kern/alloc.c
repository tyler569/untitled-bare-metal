#include "assert.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/kernel.h"
#include "kern/mem.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/bootinfo.h"

#define MAX_EXTENTS 32
#define MAX_REGIONS 64

struct physical_extent extents[MAX_EXTENTS];
size_t extent_count = MAX_EXTENTS;

struct power_of_two_region
{
  uintptr_t addr;
  uint8_t size_bits;
  bool in_kernel_use;
  bool in_user_use;
  bool for_pages;
  size_t watermark;
};

struct power_of_two_region regions[MAX_REGIONS];
size_t region_count = 0;

void
allocate_aligned_regions (struct physical_extent *extent)
{
  size_t len = extent->len;
  uintptr_t acc = extent->start;
  while (len > 0)
    {
      // find the largest power of 2 that fits in len
      int zeros = __builtin_clzll (len);
      size_t max_len = 1 << (63 - zeros);

      assert (max_len <= len);
      assert (max_len >> 12 > 0);

      printf ("  %p: %zu pages\n", (void *)acc, max_len >> 12);

      regions[region_count].addr = acc;
      regions[region_count].size_bits = 63 - zeros - 12;
      region_count++;

      acc += max_len;
      len -= max_len;
    }
}

void
create_init_untyped_caps (cte_t *base, size_t *count,
                          struct untyped_desc *desc)
{
  size_t n = MIN (*count, region_count);
  size_t cap_i = 0;
  for (size_t i = 0; i < n; i++)
    {
      if (regions[i].in_kernel_use)
        continue;
      regions[i].in_user_use = true;

      //
      // TODO PAGE_BITS
      //
      base[cap_i++].cap
          = cap_untyped_new (regions[i].addr, regions[i].size_bits + 12);

      desc[i].base = regions[i].addr;
      desc[i].size_bits = regions[i].size_bits + 12;
    }

  *count = cap_i;
}

int
compare_power_of_two_regions (const struct power_of_two_region *a,
                              const struct power_of_two_region *b)
{
  return a->size_bits - b->size_bits;
}

void
init_page_mmap ()
{
  get_physical_extents (extents, &extent_count);

  for (size_t i = 0; i < extent_count; i++)
    allocate_aligned_regions (&extents[i]);

  qsort (regions, region_count, sizeof (struct power_of_two_region),
         (int (*) (const void *, const void *))compare_power_of_two_regions);
}

uintptr_t
alloc_page ()
{
  // if there is already a kernel region being used to allocate pages, use it
  for (size_t i = 0; i < region_count; i++)
    {
      struct power_of_two_region *region = &regions[i];
      size_t size = PAGE_SIZE << region->size_bits;
      if (region->in_kernel_use && region->for_pages
          && region->watermark < size - PAGE_SIZE)
        {
          uintptr_t addr = region->addr + region->watermark;
          region->watermark += PAGE_SIZE;
          // printf ("Allocating page at %p\n", (void *)addr);
          return addr;
        }
    }

  // otherwise, find a region that is not in use and use it, prefering
  // the smallest available region
  for (size_t i = 0; i < region_count; i++)
    {
      struct power_of_two_region *region = &regions[i];
      if (!region->in_kernel_use && !region->in_user_use)
        {
          region->in_kernel_use = true;
          region->for_pages = true;
          region->watermark = PAGE_SIZE;
          // printf ("Allocating page at %p\n", (void *)region->addr);
          return region->addr;
        }
    }

  assert (0 && "No more pages available");
}

void *
kmem_alloc (size_t size)
{
  size = ALIGN_UP (size, 16);

  // if there is already a kernel region of sufficient size being used to
  // allocate memory, use it
  for (size_t i = 0; i < region_count; i++)
    {
      struct power_of_two_region *region = &regions[i];
      size_t region_size = 1ul << region->size_bits;
      if (region->in_kernel_use && !region->for_pages
          && region->watermark < region_size - size)
        {
          uintptr_t addr = region->addr + region->watermark;
          region->watermark += size;
          return (void *)direct_map_of (addr);
        }
    }

  // otherwise, find a region that is not in use and use it, prefering
  // the smallest available region
  for (size_t i = 0; i < region_count; i++)
    {
      struct power_of_two_region *region = &regions[i];
      size_t region_size = 1ul << region->size_bits;
      if (!region->in_kernel_use && !region->in_user_use && !region->for_pages
          && region_size >= size)
        {
          region->in_kernel_use = true;
          region->for_pages = false;
          region->watermark = size;
          return (void *)direct_map_of (region->addr);
        }
    }

  return nullptr;
}
