#include "stddef.h"
#include "stdint.h"
#include "sys/bootinfo.h"
#include "sys/types.h"

#include "./lib.h"

constexpr size_t BITMAP_SIZE = 512;
constexpr size_t BITS_PER_WORD = 64;
uint64_t bitmap[BITMAP_SIZE];

static inline void
cptr_set_range (cptr_t start, cptr_t end)
{
  for (size_t i = start; i < end; i++)
    {
      size_t idx = i / BITS_PER_WORD;
      size_t bit = i % BITS_PER_WORD;
      bitmap[idx] |= (1ULL << bit);
    }
}

static inline void
cptr_clear_range (cptr_t start, cptr_t end)
{
  for (size_t i = start; i < end; i++)
    {
      size_t idx = i / BITS_PER_WORD;
      size_t bit = i % BITS_PER_WORD;
      bitmap[idx] &= ~(1ULL << bit);
    }
}

void
cptr_alloc_init (struct boot_info *bi)
{
  cptr_set_range (0, BIT (INIT_CNODE_SIZE_BITS));
  cptr_clear_range (bi->empty_range.start, bi->empty_range.end);
}

cptr_t
cptr_alloc ()
{
  for (size_t i = 0; i < BITMAP_SIZE; i++)
    if (bitmap[i] != 0xFFFFFFFFFFFFFFFF)
      for (size_t j = 0; j < 64; j++)
        if ((bitmap[i] & (1 << j)) == 0)
          {
            bitmap[i] |= (1 << j);
            return (i * 64) + j;
          }
  return -1;
}

cptr_t
cptr_alloc_range (size_t n)
{
  for (size_t i = 0; i < BITMAP_SIZE; i++)
    if (bitmap[i] != 0xFFFFFFFFFFFFFFFF)
      for (size_t j = 0; j < 64; j++)
        for (size_t j = 0; j < BITS_PER_WORD; j++)
          {
            // Check if there is enough space for `n` bits starting at position
            // (i * 64 + j).
            bool found = true;
            for (size_t k = 0; k < n; k++)
              {
                size_t idx = i + ((j + k) / BITS_PER_WORD);
                size_t bit = (j + k) % BITS_PER_WORD;

                if (idx >= BITMAP_SIZE || (bitmap[idx] & (1ULL << bit)) != 0)
                  {
                    found = false;
                    break;
                  }
              }

            if (found)
              {
                // Mark the range as allocated.
                for (size_t k = 0; k < n; k++)
                  {
                    size_t idx = (i * BITS_PER_WORD + j + k) / BITS_PER_WORD;
                    size_t bit = (j + k) % BITS_PER_WORD;
                    bitmap[idx] |= (1ULL << bit);
                  }

                return (i * BITS_PER_WORD) + j;
              }
          }
  return -1;
}

void
cptr_free (cptr_t cptr)
{
  bitmap[cptr / 64] &= ~(1 << (cptr % 64));
}
