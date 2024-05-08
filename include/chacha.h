#pragma once

#include "stddef.h"
#include "stdint.h"

struct chacha
{
  uint32_t counter;
  unsigned char key[32];
  unsigned char nonce[12];
};

void xor_chacha (struct chacha *cc, unsigned char *buf, size_t len,
                 int rounds);

#define xor_chacha(chacha, buf, len) xor_chacha (chacha, buf, len, 20)

void chacha_rand (struct chacha *cc, unsigned char *buf, size_t len);
