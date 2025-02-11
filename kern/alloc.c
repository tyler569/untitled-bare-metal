#include "assert.h"
#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/kernel.h"
#include "kern/mem.h"
#include "pci.h"
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
  // size_t len = extent->len;
  // uintptr_t acc = extent->start;
  // while (len > 0)
  //   {
  //     // find the largest power of 2 that fits in len
  //     int zeros = __builtin_clzll (len);
  //     size_t max_len = 1 << (63 - zeros);

  //     assert (max_len <= len);
  //     assert (max_len >> 12 > 0);

  //     regions[region_count].addr = acc;
  //     regions[region_count].size_bits = 63 - zeros - 12;
  //     region_count++;

  //     acc += max_len;
  //     len -= max_len;
  //   }

  uintptr_t start = extent->start;
  uintptr_t end = extent->start + extent->len;

  while (start < end)
    {
      // Determine the largest power-of-two block naturally aligned at 'start'
      size_t block = start & -start; // This is a power of two.

      // If the block goes past 'end', reduce its size until it fits.
      while (start + block > end)
        block >>= 1; // Move to the next smaller power of two.

      // Process this block (e.g. record it, use it, etc.)
      // process_block(start, block);
      regions[region_count].addr = start;
      regions[region_count].size_bits = 63 - __builtin_clzll (block) - 12;
      region_count++;

      // Move to the next block.
      start += block;
    }
}

void
create_init_untyped_caps (cte_t *base, size_t *count,
                          struct untyped_desc *desc)
{
  size_t n = *count;
  size_t cap_i = 0;
  for (size_t i = 0; i < n; i++)
    {
      if (regions[i].in_kernel_use)
        continue;

      regions[i].in_user_use = true;

      base[cap_i].cap
          = cap_untyped_new (regions[i].addr, regions[i].size_bits + 12);

      desc[cap_i].base = regions[i].addr;
      desc[cap_i].size_bits = regions[i].size_bits + 12;

      cap_i++;

      if (i >= region_count - 1)
        break;
    }

  *count = cap_i;
}

void
create_init_untyped_device_caps (cte_t *base, size_t *count,
                                 struct untyped_desc *desc)
{
  size_t n = *count;
  size_t cap_i = 0;

  for (uint32_t bus = 0; bus < 256; bus++)
    for (uint32_t dev = 0; dev < 32; dev++)
      for (uint32_t func = 0; func < 8; func++)
        {
          uint32_t addr = pci_addr (bus, dev, func, 0);
          uint32_t id = read_pci_l (addr);
          if (id == 0xffffffff)
            continue;

          for (uint32_t i = 0; i < 6; i++)
            {
              uint32_t bar = read_pci_l (addr + PCI_BAR0 + i * 4);
              if (bar == 0 || bar & 1)
                continue;

              write_pci_l (addr + PCI_BAR0 + i * 4, 0xffffffff);
              uint32_t mask = read_pci_l (addr + PCI_BAR0 + i * 4);
              write_pci_l (addr + PCI_BAR0 + i * 4, bar);

              bar &= 0xfffffff0;
              mask &= 0xfffffff0;
              size_t size = ~mask + 1;
              size_t size_bits = 63 - __builtin_clzll (size);

              cap_t cap = cap_untyped_device_new (bar, size_bits);
              base[cap_i].cap = cap;

              desc[cap_i].base = bar;
              desc[cap_i].size_bits = size_bits;
              desc[cap_i].is_device = true;

              cap_i++;

              if (cap_i >= n)
                goto done;
            }
        }

done:
  *count = cap_i;
}

int
power_of_two_region_compare (const void *a, const void *b)
{
  return ((struct power_of_two_region *)b)->size_bits
         - ((struct power_of_two_region *)a)->size_bits;
}

void
init_page_mmap ()
{
  get_physical_extents (extents, &extent_count);

  for (size_t i = 0; i < extent_count; i++)
    allocate_aligned_regions (&extents[i]);

  // qsort (regions, region_count, sizeof (struct power_of_two_region),
  //        power_of_two_region_compare);
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
