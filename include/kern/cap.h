#pragma once

#include "sys/types.h"

// clang-format off

#define BITS_POINTER_MASK 0x0000'ffff'ffff'fffc

static inline void *
bits_pointer (word_t bits)
{
  word_t ptr_bits = bits & BITS_POINTER_MASK;
  if (ptr_bits & 0x8000'0000'0000)
    return (void *)(ptr_bits | 0xffff'0000'0000'0000);
  else
    return (void *)ptr_bits;
}

static inline void
set_bits_pointer (word_t *bits, void *ptr)
{
  *bits &= ~BITS_POINTER_MASK;
  *bits |= (word_t)ptr & BITS_POINTER_MASK;
}

// clang-format on

enum capability_type : unsigned char
{
  CAP_NULL,
  CAP_UNTYPED,
  CAP_ENDPOINT,
  CAP_CNODE,
  CAP_VSPACE,
  CAP_TCB,

  CAP_PML4,
  CAP_PDPT,
  CAP_PD,
  CAP_PT,

  CAP_FRAME,
};

union capability
{
  word_t words[2];
  struct
  {
    word_t type : 5;
    word_t rights : 5;
    word_t size_bits : 6;
    word_t ptr : 46;
    word_t reserved : 2;
    word_t badge;
  };
};

typedef union capability cap_t;

static_assert (sizeof (cap_t) == 16, "cap_t size is not 16 bytes");

static inline void *
cap_ptr (cap_t cap)
{
  return bits_pointer (cap.words[0]);
}

static inline word_t
cap_type (cap_t cap)
{
  return cap.type;
}

static inline void
cap_set_ptr (cap_t *cap, void *ptr)
{
  set_bits_pointer (&cap->words[0], ptr);
}

struct cdt_node
{
  word_t words[2];
};

typedef struct cdt_node cdt_node_t;

static inline cdt_node_t
cdt_new (cdt_node_t *prev, cdt_node_t *next)
{
  cdt_node_t cdt;
  cdt.words[0] = (word_t)prev;
  cdt.words[1] = (word_t)next;
  return cdt;
}

static inline cdt_node_t *
cdt_prev (cdt_node_t *cdt)
{
  return (cdt_node_t *)cdt->words[0];
}

static inline cdt_node_t *
cdt_next (cdt_node_t *cdt)
{
  return (cdt_node_t *)cdt->words[1];
}

static inline void
cdt_set_prev (cdt_node_t *cdt, cdt_node_t *prev)
{
  set_bits_pointer (&cdt->words[0], prev);
}

static inline void
cdt_set_next (cdt_node_t *cdt, cdt_node_t *next)
{
  set_bits_pointer (&cdt->words[1], next);
}

struct cte
{
  cap_t cap;
  cdt_node_t cdt;
};

typedef struct cte cte_t;

static_assert (sizeof (cte_t) == 32, "cte_t size is not 32 bytes");

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
  cap_set_ptr (&cap, endpoint);
  return cap;
}

static inline cap_t
cap_cnode_new (void *cnode, uintptr_t size_bits)
{
  cap_t cap
      = { .type = CAP_CNODE, .size_bits = size_bits, .ptr = (uintptr_t)cnode };
  return cap;
}

static inline cap_t
cap_vspace_new (uintptr_t root_phy)
{
  cap_t cap = { .type = CAP_VSPACE, .badge = root_phy };
  return cap;
}

static inline cap_t
cap_tcb_new (void *tcb)
{
  cap_t cap = { .type = CAP_TCB };
  cap_set_ptr (&cap, tcb);
  return cap;
}

static inline cap_t
cap_pml4_new (uintptr_t pml4_phy)
{
  cap_t cap = { .type = CAP_PML4 };
  cap_set_ptr (&cap, (void *)pml4_phy);
  return cap;
}

static inline cap_t
cap_pdpt_new (uintptr_t pdpt_phy)
{
  cap_t cap = { .type = CAP_PDPT };
  cap_set_ptr (&cap, (void *)pdpt_phy);
  return cap;
}

static inline cap_t
cap_pd_new (uintptr_t pd_phy)
{
  cap_t cap = { .type = CAP_PD };
  cap_set_ptr (&cap, (void *)pd_phy);
  return cap;
}

static inline cap_t
cap_pt_new (uintptr_t pt_phy)
{
  cap_t cap = { .type = CAP_PT };
  cap_set_ptr (&cap, (void *)pt_phy);
  return cap;
}

static inline cap_t
cap_frame_new (uintptr_t frame_phy)
{
  cap_t cap = { .type = CAP_FRAME };
  cap_set_ptr (&cap, (void *)frame_phy);
  return cap;
}

static inline cap_t
lookup_cap (cap_t cspace_root, word_t index, word_t depth)
{
  assert (depth == 64); // for now
  assert (cap_type (cspace_root) == CAP_CNODE);

  cte_t *cte = (cte_t *)cap_ptr (cspace_root);
  size_t length = 1 << cspace_root.size_bits;
  assert (index < length);

  return cte[index].cap;
}