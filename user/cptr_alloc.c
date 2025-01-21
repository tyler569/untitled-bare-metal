#include "stddef.h"
#include "stdint.h"
#include "sys/bootinfo.h"
#include "sys/types.h"

#include "./lib.h"

constexpr size_t bitmap_size = 512;
constexpr size_t bits_per_word = 64;
uint64_t bitmap[bitmap_size];

cptr_t cptr_alloc()
{
  for (size_t i = 0; i < bitmap_size; i++)
    if (bitmap[i] != 0xFFFFFFFFFFFFFFFF)
      for (size_t j = 0; j < 64; j++)
        if ((bitmap[i] & (1 << j)) == 0)
          {
            bitmap[i] |= (1 << j);
            return (i * 64) + j;
          }
  return -1;
}

cptr_t cptr_alloc_range (size_t n)
{
  for (size_t i = 0; i < bitmap_size; i++)
    if (bitmap[i] != 0xFFFFFFFFFFFFFFFF)
      for (size_t j = 0; j < 64; j++)
        for (size_t j = 0; j < bits_per_word; j++)
          {
            // Check if there is enough space for `n` bits starting at position (i * 64 + j).
            bool found = true;
            for (size_t k = 0; k < n; k++)
              {
                size_t idx = i + ((j + k) / bits_per_word);
                size_t bit = (j + k) % bits_per_word;

                if (idx >= bitmap_size || (bitmap[idx] & (1ULL << bit)) != 0)
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
                    size_t idx = (i * bits_per_word + j + k) / bits_per_word;
                    size_t bit = (j + k) % bits_per_word;
                    bitmap[idx] |= (1ULL << bit);
                  }

                return (i * bits_per_word) + j;
              }
          }
  return -1;
}

void cptr_free(cptr_t cptr)
{
  bitmap[cptr / 64] &= ~(1 << (cptr % 64));
}
