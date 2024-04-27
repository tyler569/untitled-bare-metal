#include "assert.h"
#include "kernel.h"
#include "list.h"
#include "string.h"
#include "sys/lock.h"
#include "sys/mem.h"

#define PAGE_SIZE 4096

struct slab_cache
{
  struct list_head list;

  struct list_head slabs_full;
  struct list_head slabs_partial;
  struct list_head slabs_free;

  spin_lock_t lock;

  size_t object_size;
  size_t slab_page_count;
};

void
slab_cache_init (struct slab_cache *cache, size_t size)
{
  list_init (&cache->list);
  list_init (&cache->slabs_full);
  list_init (&cache->slabs_partial);
  list_init (&cache->slabs_free);

  cache->object_size = size;

  if (size < 64)
    cache->slab_page_count = 1;
  else
    cache->slab_page_count = 1 + (size * 8 - 1) / PAGE_SIZE;
}

#define BITMAP_SIZE (sizeof (uint64_t) * 4)

static size_t
obj_per_page (struct slab_cache *cache)
{
  return (PAGE_SIZE * cache->slab_page_count - BITMAP_SIZE)
         / cache->object_size;
}

static void *
obj_addr (page_t *page, size_t i, struct slab_cache *cache)
{
  uintptr_t addr = page->slab_kernel_addr;
  assert (addr);

  return (void *)((uint8_t *)addr + BITMAP_SIZE + i * cache->object_size);
}

static bool
obj_is_free (page_t *page, size_t i)
{
  uintptr_t addr = page->slab_kernel_addr;
  assert (addr);

  return (((uint64_t *)addr)[i / 64] & (1ul << (i % 64))) == 0;
}

static void
obj_set_bitmap (page_t *page, size_t i)
{
  uintptr_t addr = page->slab_kernel_addr;
  assert (addr);

  ((uint64_t *)addr)[i / 64] |= (1ul << (i % 64));
}

static void
obj_clr_bitmap (page_t *page, size_t i)
{
  uintptr_t addr = page->slab_kernel_addr;
  assert (addr);

  ((uint64_t *)addr)[i / 64] &= ~(1ul << (i % 64));
}

static size_t
page_free_index (page_t *page, size_t obj_per_page)
{
  for (size_t i = 0; i < obj_per_page; i++)
    {
      if (obj_is_free (page, i))
        return i;
    }

  assert (0 && "no free index found");
}

static void
up_ref_slab (struct slab_cache *cache, page_t *slab)
{
  slab->slab_objects++;

  if (slab->slab_objects == obj_per_page (cache))
    {
      list_remove (&slab->list);
      list_insert_after (&slab->list, &cache->slabs_full);
    }
}

static void
down_ref_slab (struct slab_cache *cache, page_t *slab)
{
  if (slab->slab_objects == obj_per_page (cache))
    {
      list_remove (&slab->list);
      list_insert_after (&slab->list, &cache->slabs_partial);
    }

  slab->slab_objects--;

  if (slab->slab_objects == 0)
    {
      list_remove (&slab->list);
      list_insert_after (&slab->list, &cache->slabs_free);
    }
}

void
slab_cache_grow (struct slab_cache *cache)
{
  size_t slab_size = cache->slab_page_count * PAGE_SIZE;
  uintptr_t slab_vm_addr = alloc_kernel_vm (slab_size);
  uintptr_t vm_root = get_vm_root ();

  page_t *slab_head = nullptr;

  for (size_t i = 0; i < cache->slab_page_count; i++)
    {
      page_t *page;
      uintptr_t addr = alloc_page_s (&page);

      assert (addr && "out of memory");

      add_vm_mapping (vm_root, slab_vm_addr + i * PAGE_SIZE, addr,
                      PTE_PRESENT | PTE_WRITE);

      list_init (&page->list);

      if (i == 0)
        {
          slab_head = page;
          page->flags = PAGE_USED | PAGE_SLAB | PAGE_HEAD;
        }
      else
        page->flags = PAGE_USED | PAGE_SLAB | PAGE_MEMBER;

      page->slab_head = slab_head;
      page->slab_kernel_addr = slab_vm_addr;
      page->slab_cache = cache;
    }

  memset ((void *)slab_vm_addr, 0, slab_size);

  list_insert_after (&slab_head->list, &cache->slabs_free);
}

void *
slab_alloc (struct slab_cache *cache)
{
  spin_lock (&cache->lock);

  page_t *slab = nullptr;

  if (!list_empty (&cache->slabs_partial))
    slab = CONTAINER_OF (list_first (&cache->slabs_partial), page_t, list);

  if (!slab)
    {
      if (list_empty (&cache->slabs_free))
        slab_cache_grow (cache);

      slab = CONTAINER_OF (list_first (&cache->slabs_free), page_t, list);
      list_remove (&slab->list);
      list_insert_after (&slab->list, &cache->slabs_partial);
    }

  size_t per_page = obj_per_page (cache);
  size_t index = page_free_index (slab, per_page);
  void *obj = obj_addr (slab, index, cache);
  obj_set_bitmap (slab, index);
  up_ref_slab (cache, slab);

  spin_unlock (&cache->lock);

  return obj;
}

void
slab_free (struct slab_cache *cache, void *ptr)
{
  spin_lock (&cache->lock);

  uintptr_t vm_root = get_vm_root ();
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t phys_addr = resolve_vm_mapping (vm_root, addr);

  page_t *page = get_page_struct (phys_addr);
  page_t *slab = page->slab_head;

  uintptr_t slab_addr = slab->slab_kernel_addr;
  size_t index = (addr - slab_addr - BITMAP_SIZE) / cache->object_size;

  obj_clr_bitmap (slab, index);
  down_ref_slab (cache, slab);

  spin_unlock (&cache->lock);
}

struct malloc_bucket
{
  const size_t size;
  struct slab_cache cache;
};

static struct malloc_bucket buckets[] = {
  { .size = 16 },   { .size = 24 },   { .size = 32 },   { .size = 48 },
  { .size = 64 },   { .size = 96 },   { .size = 128 },  { .size = 192 },
  { .size = 256 },  { .size = 384 },  { .size = 512 },  { .size = 768 },
  { .size = 1024 }, { .size = 1536 }, { .size = 2048 }, { .size = 3072 }
};

static size_t
sat_sub64 (size_t a, size_t b)
{
  size_t res = a - b;
  res &= -(res <= a);

  return res;
}

size_t
get_bucket_index_analytic (size_t size)
{
  if (__builtin_expect (size < 2, 0))
    return 0;

  size_t sm1 = size - 1;
  size_t l2 = 63 - __builtin_clzl (sm1);
  size_t bit2 = sm1 & (1ul << (l2 - 1));

  return sat_sub64 (l2 * 2 + (bit2 == 0 ? 0 : 1), 7);
}

void
init_kmem_alloc ()
{
  for (size_t i = 0; i < ARRAY_SIZE (buckets); i++)
    {
      struct slab_cache *cache = &buckets[i].cache;
      slab_cache_init (cache, buckets[i].size);
    }
}

void *
kmem_page_alloc (size_t size)
{
  size_t n_pages = ALIGN_UP (size, PAGE_SIZE) / PAGE_SIZE;
  uintptr_t vm_root = get_vm_root ();
  uintptr_t vm_addr = alloc_kernel_vm (n_pages * PAGE_SIZE);
  page_t *alloc_head = nullptr;

  for (size_t i = 0; i < n_pages; i++)
    {
      page_t *page;
      uintptr_t addr = alloc_page_s (&page);
      add_vm_mapping (vm_root, vm_addr + i * PAGE_SIZE, addr,
                      PTE_PRESENT | PTE_WRITE);

      if (i == 0)
        {
          alloc_head = page;
          page->flags = PAGE_USED | PAGE_ALLOC | PAGE_HEAD;
        }
      else
        page->flags = PAGE_USED | PAGE_ALLOC | PAGE_MEMBER;

      page->alloc_head = alloc_head;
      page->alloc_kernel_addr = vm_addr + i * PAGE_SIZE;
      page->alloc_pages = n_pages;
    }

  return (void *)vm_addr;
}

void
kmem_page_free (page_t *page)
{
  uintptr_t vm_root = get_vm_root ();
  uintptr_t vm_addr = page->alloc_kernel_addr;
  size_t n_pages = page->alloc_pages;

  for (size_t i = 0; i < n_pages; i++)
    {
      uintptr_t addr = resolve_vm_mapping (vm_root, vm_addr + i * PAGE_SIZE);
      free_page (addr);
      add_vm_mapping (vm_root, vm_addr + i * PAGE_SIZE, 0, 0);
    }
}

void *
kmem_alloc (size_t size)
{
  if (size >= 4096)
    return kmem_page_alloc (size);

  size_t index = get_bucket_index_analytic (size);
  return slab_alloc (&buckets[index].cache);
}

void
kmem_free (void *ptr)
{
  uintptr_t vm_root = get_vm_root ();
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t phys_addr = resolve_vm_mapping (vm_root, addr);

  page_t *page = get_page_struct (phys_addr);

  if (page->flags & PAGE_ALLOC)
    kmem_page_free (page);
  else if (page->flags & PAGE_SLAB)
    slab_free (page->slab_cache, ptr);
  else
    assert (0 && "invalid free");
}
