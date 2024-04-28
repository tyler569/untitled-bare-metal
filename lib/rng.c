#include "chacha20.h"
#include "string.h"

chacha20 chacha_state;

void
init_random (uint64_t seed)
{
  chacha_state.counter = 1;
  memcpy (chacha_state.key, &seed, sizeof (seed));
}

uint64_t
random_u64 ()
{
  uint64_t r;
  xor_chacha20 (&chacha_state, (unsigned char *)&r, sizeof (r));
  return r;
}
