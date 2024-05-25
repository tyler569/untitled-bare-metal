#pragma once

#include "sys/types.h"

static inline void *bits_pointer (word_t bits)
{
  word_t ptr_bits = bits & 0x0000'ffff'ffff'fffc;
  if (ptr_bits & 0x8000'0000'0000) {
    return (void *)(ptr_bits | 0xffff'0000'0000'0000);
  } else {
    return (void *)ptr_bits;
  }
}

static inline void set_bits_pointer (word_t *bits, void *ptr)
{
  *bits &= ~0x0000'ffff'ffff'fffc;
  *bits |= (word_t)ptr & 0x0000'ffff'ffff'fffc;
}

enum capability_type : unsigned char
{
  CAP_NULL,
  CAP_UNTYPED,
  CAP_ENDPOINT,
  CAP_CNODE,
  CAP_VSPACE,
  CAP_TCB,
};

struct capability {
    union {
        word_t words[2];
        struct {
            word_t type : 5;
            word_t rights : 3;
            word_t size_bits : 8;
            word_t ptr : 48;
            word_t badge;
        };
    };
};

typedef struct capability cap_t;

static_assert(sizeof(cap_t) == 16, "cap_t size is not 16 bytes");

static inline void *cap_ptr(cap_t cap) {
    return bits_pointer(cap.words[0]);
}

static inline void cap_set_ptr(cap_t *cap, void *ptr) {
    set_bits_pointer(&cap->words[0], ptr);
}

struct cdt_node {
    word_t words[2];
};

typedef struct cdt_node cdt_t;

static inline cdt_t cdt_new(cdt_t *prev, cdt_t *next) {
    cdt_t cdt;
    cdt.words[0] = (word_t)prev;
    cdt.words[1] = (word_t)next;
    return cdt;
}

static inline cdt_t *cdt_prev(cdt_t *cdt) {
    return (cdt_t *)cdt->words[0];
}

static inline cdt_t *cdt_next(cdt_t *cdt) {
    return (cdt_t *)cdt->words[1];
}

static inline void cdt_set_prev(cdt_t *cdt, cdt_t *prev) {
    set_bits_pointer(&cdt->words[0], prev);
}

static inline void cdt_set_next(cdt_t *cdt, cdt_t *next) {
    set_bits_pointer(&cdt->words[1], next);
}

struct cte {
    struct capability cap;
    struct cdt_node cdt;
};

typedef struct cte cte_t;

static_assert(sizeof(cte_t) == 32, "cte_t size is not 32 bytes");

static inline cap_t
cap_null_new ()
{
  cap_t cap = { .type = CAP_NULL };
  return cap;
}

static inline cap_t
cap_untyped_new (uintptr_t paddr, uintptr_t size_bits)
{
  cap_t cap = { .type = CAP_UNTYPED, .size_bits = size_bits, .ptr = paddr };
  return cap;
}

static inline cap_t
cap_endpoint_new (void *endpoint, uintptr_t badge)
{
  cap_t cap = { .type = CAP_ENDPOINT, .badge = badge };
  cap_set_ptr(&cap, endpoint);
  return cap;
}

static inline cap_t
cap_cnode_new (void *cnode, uintptr_t size_bits)
{
  cap_t cap = { .type = CAP_CNODE, .size_bits = size_bits, .ptr = (uintptr_t)cnode };
  return cap;
}

static inline cap_t cap_vspace_new (uintptr_t root_phy)
{
  cap_t cap = { .type = CAP_VSPACE, .badge = root_phy };
  return cap;
}

static inline cap_t cap_tcb_new (void *tcb)
{
  cap_t cap = { .type = CAP_TCB };
  cap_set_ptr(&cap, tcb);
  return cap;
}
