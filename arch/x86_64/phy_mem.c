#include "kern/kernel.h"
#include "kern/mem.h"
#include "limine.h"
#include "stdio.h"
#include "x86_64.h"

static struct limine_memmap_request mmapinfo = {
  .id = LIMINE_MEMMAP_REQUEST,
};

static struct limine_hhdm_request hhdminfo = {
  .id = LIMINE_HHDM_REQUEST,
};

const char *limine_memmap_type_str[] = {
  [LIMINE_MEMMAP_USABLE] = "Usable",
  [LIMINE_MEMMAP_RESERVED] = "Reserved",
  [LIMINE_MEMMAP_ACPI_RECLAIMABLE] = "ACPI reclaimable",
  [LIMINE_MEMMAP_ACPI_NVS] = "ACPI NVS",
  [LIMINE_MEMMAP_BAD_MEMORY] = "Bad memory",
  [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "Bootloader reclaimable",
  [LIMINE_MEMMAP_KERNEL_AND_MODULES] = "Kernel and modules",
  [LIMINE_MEMMAP_FRAMEBUFFER] = "Framebuffer",
};

bool limine_memmap_type_available[] = {
  [LIMINE_MEMMAP_USABLE] = true,
  [LIMINE_MEMMAP_ACPI_RECLAIMABLE] = true,
  [LIMINE_MEMMAP_ACPI_NVS] = true,
  [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = true,
  [LIMINE_MEMMAP_KERNEL_AND_MODULES] = true,
};

uintptr_t
direct_map_of (uintptr_t addr)
{
  static uintptr_t hhdm_cache = 0;
  if (!hhdm_cache)
    hhdm_cache = volatile_read (hhdminfo.response)->offset;
  return addr | hhdm_cache;
}

uintptr_t
physical_of (uintptr_t addr)
{
  return (uintptr_t)addr - direct_map_of (0);
}

void
get_physical_extents (struct physical_extent *extents, size_t *extent_count)
{
  size_t max_extents = *extent_count;
  *extent_count = 0;

  struct limine_memmap_response *resp = volatile_read (mmapinfo.response);

  for (size_t i = 0; i < resp->entry_count; i++)
    {
      struct limine_memmap_entry *entry = resp->entries[i];

      if (entry->type == LIMINE_MEMMAP_USABLE && *extent_count < max_extents)
        {
          extents[*extent_count].start = entry->base;
          extents[*extent_count].len = entry->length;
          (*extent_count)++;
        }
    }
}
