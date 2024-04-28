#include "chacha20.h"
#include "stdio.h"
#include "string.h"

static inline uint32_t
rol (uint32_t x, int n)
{
  return (x << n) | (x >> (32 - n));
}

#define QUARTER_ROUND(a, b, c, d)                                             \
  do                                                                          \
    {                                                                         \
      state[a] += state[b];                                                   \
      state[d] = rol (state[d] ^ state[a], 16);                               \
      state[c] += state[d];                                                   \
      state[b] = rol (state[b] ^ state[c], 12);                               \
      state[a] += state[b];                                                   \
      state[d] = rol (state[d] ^ state[a], 8);                                \
      state[c] += state[d];                                                   \
      state[b] = rol (state[b] ^ state[c], 7);                                \
    }                                                                         \
  while (0)

static void
do_chacha20_block (uint32_t state[static 16])
{
  uint32_t state_copy[16];
  memcpy (state_copy, state, sizeof (state_copy));

  for (size_t i = 0; i < 10; i++)
    {
      QUARTER_ROUND (0, 4, 8, 12);
      QUARTER_ROUND (1, 5, 9, 13);
      QUARTER_ROUND (2, 6, 10, 14);
      QUARTER_ROUND (3, 7, 11, 15);
      QUARTER_ROUND (0, 5, 10, 15);
      QUARTER_ROUND (1, 6, 11, 12);
      QUARTER_ROUND (2, 7, 8, 13);
      QUARTER_ROUND (3, 4, 9, 14);
    }

  for (size_t i = 0; i < 16; i++)
    state[i] += state_copy[i];
}

static void
print_chacha20_state (uint32_t state[static 16])
{
  for (size_t i = 0; i < 16; i++)
    {
      printf ("%08x ", state[i]);
      if (i % 4 == 3)
        printf ("\n");
    }
}

void
xor_chacha20 (chacha20 *cc, unsigned char *buf, size_t len)
{
  size_t count = 0;
  do
    {
      uint32_t state[16] = { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
      memcpy (state + 4, cc->key, 32);
      state[12] = cc->counter;
      memcpy (state + 13, cc->nonce, 12);

      do_chacha20_block (state);

      size_t i;
      for (i = 0; i < 64 && count + i < len; i++)
        buf[count + i] ^= ((unsigned char *)state)[i] & 0xff;
      count += i;

      cc->counter++;
    }
  while (count < len);
}