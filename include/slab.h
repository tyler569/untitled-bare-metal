#pragma once

#include "list.h"
#include "spinlock.h"
#include "stddef.h"

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

void init_slab_cache (struct slab_cache *, size_t object_size);

void *slab_alloc (struct slab_cache *);
void slab_free (struct slab_cache *, void *object);
