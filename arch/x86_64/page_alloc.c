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

typedef struct page page_t;

static spin_lock_t page_lock;

static page_t *global_page_map;
static size_t global_page_count;

static page_t *free_list_head;
static page_t *free_bump_cursor;

static volatile struct limine_memmap_request limine_memmap_request = {
  .id = LIMINE_MEMMAP_REQUEST,
};

static volatile struct limine_hhdm_request limine_hhdm_request = {
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
  return limine_hhdm_request.response->offset;
}

page_t *
alloc_page_map (size_t num_pages)
{
  struct limine_memmap_response *resp = limine_memmap_request.response;

  for (size_t i = 0; i < resp->entry_count; i++)
    {
      struct limine_memmap_entry *entry = resp->entries[i];

      if (entry->type != LIMINE_MEMMAP_USABLE)
        continue;

      uintptr_t base = entry->base;

      if (entry->length < num_pages * sizeof (page_t))
        continue;

      page_t *page = (page_t *)(base | limine_hhdm ());

      memset (page, 0, num_pages * sizeof (page_t));

      return page;
    }

  return nullptr;
}

page_t *
get_page (uintptr_t addr)
{
  size_t index = addr / PAGE_SIZE;

  if (index >= global_page_count)
    return nullptr;

  return &global_page_map[index];
}

void
fill_page_map ()
{
  struct limine_memmap_response *resp = limine_memmap_request.response;

  for (size_t i = 0; i < resp->entry_count; i++)
    {
      struct limine_memmap_entry *entry = resp->entries[i];

      if (entry->type != LIMINE_MEMMAP_USABLE)
        continue;

      for (size_t j = 0; j < entry->length / PAGE_SIZE; j++)
        {
          page_t *page = get_page (entry->base + j * PAGE_SIZE);
          page->flags = 1;
        }
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
  struct limine_memmap_response *resp = limine_memmap_request.response;

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
  global_page_count = page_struct_count;
  free_bump_cursor = global_page_map;

  fill_page_map ();
}

uintptr_t
page_addr (page_t *page)
{
  return (uintptr_t)(page - global_page_map) * PAGE_SIZE;
}

uintptr_t
alloc_page ()
{
  uintptr_t res = 0;

  spin_lock (&page_lock);

  if (free_list_head)
    {
      page_t *page = free_list_head;
      free_list_head = page->next;

      res = page_addr (page);
    }
  else
    {
      while (++free_bump_cursor < global_page_map + global_page_count)
        {
          if ((free_bump_cursor->flags & 1) == 1)
            {
              res = page_addr (free_bump_cursor);
              break;
            }
        }
    }

  spin_unlock (&page_lock);

  return res;
}

void
free_page (uintptr_t addr)
{
  spin_lock (&page_lock);

  page_t *page = get_page (addr);
  page->next = free_list_head;
  free_list_head = page;

  spin_unlock (&page_lock);
}