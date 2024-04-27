#pragma once

#include <list.h>
#include <stdint.h>
#include <stdio.h>

struct page
{
  uint64_t flags;
  struct list_head list;
  union
  {
    struct
    {
      struct page *slab_head;
      struct slab_cache *slab_cache;
      uintptr_t slab_kernel_addr;
      uint32_t slab_objects;
    };
    struct
    {
      struct page *alloc_head;
      uintptr_t alloc_kernel_addr;
      uint32_t alloc_pages;
    };
  };
};

enum page_flags : uint64_t
{
  PAGE_UNUSABLE = 1 << 63,
  PAGE_FREE = 0,
  PAGE_USED = 1 << 0,
  PAGE_SLAB = 1 << 1,
  PAGE_ALLOC = 1 << 2,
  PAGE_HEAD = 1 << 3,
  PAGE_MEMBER = 1 << 4,
};

typedef struct page page_t;

void kernel_main ();

uintptr_t alloc_page_s (page_t **page);
#define alloc_page() alloc_page_s (nullptr)
void free_page (uintptr_t);
page_t *get_page_struct (uintptr_t);

#define PTE_PRESENT (1 << 0)
#define PTE_WRITE (1 << 1)
#define PTE_USER (1 << 2)

uintptr_t get_vm_root ();
uintptr_t alloc_kernel_vm (size_t len);
void add_vm_mapping (uintptr_t root, uintptr_t virt, uintptr_t phys,
                     int flags);
uintptr_t resolve_vm_mapping (uintptr_t root, uintptr_t virt);
uintptr_t new_page_table ();

void init_kmem_alloc ();
void *kmem_alloc (size_t size);
void kmem_free (void *ptr);

[[noreturn]] void panic (const char *msg, ...);

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))

// arch-specific procedures provided by arch code.

struct frame;
typedef struct frame frame_t;

[[noreturn]] void halt_forever ();

void relax_busy_loop ();

ssize_t write_debug (FILE *, const void *str, size_t len);

void print_backtrace (frame_t *);
void print_frame (frame_t *);