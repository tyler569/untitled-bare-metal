#include "stddef.h"

void
qsort (void *base, size_t num, size_t size,
       int (*compare) (const void *, const void *))
{
  if (num < 2)
    return;

  char *base_ptr = (char *)base;
  char *base_end = base_ptr + num * size;

  // Select the pivot to be the middle element
  char *pivot = base_ptr + (num / 2) * size;
  char *left = base_ptr;
  char *right = base_end - size;

  while (left <= right)
    {
      while (compare (left, pivot) < 0)
        left += size;

      while (compare (right, pivot) > 0)
        right -= size;

      if (left <= right)
        {
          for (size_t i = 0; i < size; i++)
            {
              char tmp = left[i];
              left[i] = right[i];
              right[i] = tmp;
            }

          if (left == pivot)
            pivot = right;
          else if (right == pivot)
            pivot = left;

          left += size;
          right -= size;
        }
    }

  qsort (base_ptr, (right - base_ptr) / size + 1, size, compare);
  qsort (left, (base_end - left) / size, size, compare);
}
