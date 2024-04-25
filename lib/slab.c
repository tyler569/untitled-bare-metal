#include <stddef.h>

struct slab
{
  struct slab *next;
  struct slab *prev;
  struct slab *next_free;
  struct slab *prev_free;

  size_t free_count;
  size_t total_count;
  size_t elem_size;
  size_t elem_count;
};

struct slab_cache
{
  struct slab *slabs;
  struct slab *free_slabs;
  size_t elem_size;
  size_t elem_count;
};
