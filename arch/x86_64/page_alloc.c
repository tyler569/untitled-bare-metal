#include "kernel.h"
#include "limine.h"
#include <stdio.h>
#include <string.h>
#include <sys/lock.h>

#define PAGE_SIZE 4096

struct page
{
  uint64_t flags;
  struct page *next;
  struct page *prev;
  uint64_t reserved[5];
};

enum page_flags : uint64_t
{
  PAGE_UNUSABLE = 1 << 63,
  PAGE_FREE = 0,
  PAGE_USED = 1,
};

typedef struct page page_t;

static spin_lock_t page_lock;

static uintptr_t global_page_map_phy;

static page_t *global_page_map;
static size_t global_page_count;
static page_t *global_page_map_end;

static page_t *free_list_head;
static page_t *free_bump_cursor;

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
limine_hhdm ()
{
  static uintptr_t hhdm_cache = 0;
  if (!hhdm_cache)
    hhdm_cache = volatile_read (hhdminfo.response)->offset;
  return hhdm_cache;
}

uintptr_t
limine_reverse_hhdm (void *addr)
{
  return (uintptr_t)addr - limine_hhdm ();
}

page_t *
alloc_page_map (size_t num_pages)
{
  struct limine_memmap_response *resp = volatile_read (mmapinfo.response);

  for (size_t i = 0; i < resp->entry_count; i++)
    {
      struct limine_memmap_entry *entry = resp->entries[i];

      if (entry->type != LIMINE_MEMMAP_USABLE)
        continue;

      uintptr_t base = entry->base;

      if (entry->length < num_pages * sizeof (page_t))
        continue;

      page_t *page = (page_t *)(base | limine_hhdm ());

      return page;
    }

  return nullptr;
}

page_t *
get_page_struct (uintptr_t addr)
{
  size_t index = addr / PAGE_SIZE;

  if (index >= global_page_count)
    return nullptr;

  return &global_page_map[index];
}

void
fill_page_map ()
{
  struct limine_memmap_response *resp = volatile_read (mmapinfo.response);

  for (size_t i = 0; i < global_page_count; i++)
    global_page_map[i].flags = PAGE_UNUSABLE;

  for (size_t i = 0; i < resp->entry_count; i++)
    {
      struct limine_memmap_entry *entry = resp->entries[i];

      if (entry->type != LIMINE_MEMMAP_USABLE)
        continue;

      for (size_t j = 0; j < entry->length / PAGE_SIZE; j++)
        {
          page_t *page = get_page_struct (entry->base + j * PAGE_SIZE);
          page->flags = PAGE_FREE;
        }
    }

  for (uintptr_t addr = global_page_map_phy;
       addr < global_page_map_phy + global_page_count * sizeof (page_t);
       addr += PAGE_SIZE)
    {
      page_t *page = get_page_struct (addr);
      page->flags = PAGE_USED;
    }
}

// Print a number in SI units (e.g. 1.23 MB) out to 3 decimal places
void
print_si_fraction (size_t num)
{
  static const char *suffixes[] = { "kB", "MB", "GB", "TB" };

  if (num < 1024)
    {
      printf ("%zu B", num);
      return;
    }

  for (int i = 0; i < 4; i++)
    {
      size_t p = 1024lu << (i * 10);
      if (num < p * 1024)
        {
          if (num % p == 0)
            printf ("%zu %s", num / p, suffixes[i]);
          else
            printf ("%zu.%03zu %s", num / p, num % p * 1000 / p, suffixes[i]);
          break;
        }
    }
}

void
init_page_mmap ()
{
  struct limine_memmap_response *resp = volatile_read (mmapinfo.response);

  if (!resp)
    return;

  size_t total_pages = 0;
  size_t free_pages = 0;
  uintptr_t highest_usable_addr = 0;

  printf ("Limine memory map:\n");

  for (size_t i = 0; i < resp->entry_count; i++)
    {
      struct limine_memmap_entry *entry = resp->entries[i];

      uintptr_t top = entry->base + entry->length;
      size_t pages = entry->length / PAGE_SIZE;

      printf ("  %11lx - %11lx: %s\n", entry->base, top,
              limine_memmap_type_str[entry->type]);

      if (limine_memmap_type_available[entry->type])
        {
          total_pages += pages;
          if (top > highest_usable_addr)
            highest_usable_addr = top;
        }

      if (entry->type == LIMINE_MEMMAP_USABLE)
        free_pages += pages;
    }
  printf ("Total pages: %zu (", total_pages);
  print_si_fraction (total_pages * PAGE_SIZE);
  printf (")\n");
  printf ("Free pages: %zu (", free_pages);
  print_si_fraction (free_pages * PAGE_SIZE);
  printf (")\n");

  size_t page_struct_size = sizeof (struct page);
  size_t page_struct_count = highest_usable_addr / PAGE_SIZE;
  size_t page_map_total_size = page_struct_size * page_struct_count;

  printf ("Page struct size: (");
  print_si_fraction (page_map_total_size);
  printf (")\n");

  global_page_map = alloc_page_map (page_struct_count);
  global_page_map_phy = limine_reverse_hhdm (global_page_map);
  global_page_count = page_struct_count;
  global_page_map_end = global_page_map + page_struct_count;
  free_bump_cursor = global_page_map;

  fill_page_map ();
}

bool
page_is_free (page_t *page)
{
  return page->flags == PAGE_FREE;
}

uintptr_t
page_addr (page_t *page)
{
  return (uintptr_t)(page - global_page_map) * PAGE_SIZE;
}

uintptr_t
alloc_page ()
{
  page_t *page = nullptr;

  spin_lock (&page_lock);

  if (free_list_head)
    {
      page = free_list_head;
      free_list_head = page->next;
    }
  else
    do
      if (page_is_free (free_bump_cursor))
        {
          page = free_bump_cursor++;
          break;
        }
    while (++free_bump_cursor < global_page_map_end);

  spin_unlock (&page_lock);

  return page_addr (page);
}

void
free_page (uintptr_t addr)
{
  spin_lock (&page_lock);

  page_t *page = get_page_struct (addr);
  page->next = free_list_head;
  page->flags = PAGE_FREE;
  free_list_head = page;

  spin_unlock (&page_lock);
}
