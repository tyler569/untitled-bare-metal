#include "calculator2.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

uint64_t impl_calculator2_quit() {
  printf("calculator2: received quit\n");
  return 0;
}

uint64_t impl_calculator2_ret42() {
  return 42;
}

uint64_t impl_calculator2_double(uint64_t value) {
  return value * 2;
}

uint64_t impl_calculator2_inc(uint64_t value) {
  return value + 1;
}

uint64_t impl_calculator2_add(uint64_t a, uint64_t b) {
  return a + b;
}

int main(cptr_t endpoint_cap) {
  calculator2_server(endpoint_cap);
  return 0;
}
