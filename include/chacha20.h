#pragma once

#include "stddef.h"
#include "stdint.h"

struct chacha20
{
  uint32_t counter;
  unsigned char key[32];
  unsigned char nonce[12];
};

typedef struct chacha20 chacha20;

void xor_chacha20 (chacha20 *cc, unsigned char *buf, size_t len);
