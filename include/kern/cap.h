#pragma once

#include "assert.h"
#include "sys/cdefs.h"
#include "sys/syscall.h"
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

union capability
{
  word_t words[2];
  struct
  {
    word_t reserved : 2;
    word_t ptr : 46;
    word_t size_bits : 6;
    word_t rights : 5;
    word_t type : 5;
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

static inline void
cap_set_ptr (cap_t *cap, void *ptr)
{
  set_bits_pointer (&cap->words[0], ptr);
}

static inline word_t
cap_type (cap_t cap)
{
  return cap.type;
}

static inline word_t
cap_size (cap_t cap)
{
  return BIT (cap.size_bits);
}

static inline void
cap_set_size (cap_t *cap, word_t size_bits)
{
  cap->size_bits = size_bits;
}

static inline word_t
cap_rights (cap_t cap)
{
  return cap.rights;
}

static inline void
cap_set_rights (cap_t *cap, word_t rights)
{
  cap->rights = rights;
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
  cap_t cap = { .type = cap_null };
  return cap;
}

static inline cap_t
cap_untyped_new (uintptr_t paddr, uintptr_t size_bits)
{
  cap_t cap = { .type = cap_untyped, .size_bits = size_bits };
  cap_set_ptr (&cap, (void *)paddr);
  return cap;
}

static inline cap_t
cap_endpoint_new (void *endpoint, uintptr_t badge)
{
  cap_t cap = { .type = cap_endpoint, .badge = badge };
  cap_set_ptr (&cap, endpoint);
  return cap;
}

static inline cap_t
cap_cnode_new (void *cnode, uintptr_t size_bits)
{
  cap_t cap = { .type = cap_cnode, .size_bits = size_bits };
  cap_set_ptr (&cap, cnode);
  return cap;
}

static inline cap_t
cap_vspace_new (uintptr_t root_phy)
{
  cap_t cap = { .type = cap_vspace, .badge = root_phy };
  return cap;
}

static inline cap_t
cap_tcb_new (void *tcb)
{
  cap_t cap = { .type = cap_tcb };
  cap_set_ptr (&cap, tcb);
  return cap;
}

// static inline cap_t
// cap_pml4_new (uintptr_t pml4_phy)
// {
//   cap_t cap = { .type = cap_x86_64_pml4 };
//   cap_set_ptr (&cap, (void *)pml4_phy);
//   return cap;
// }

static inline cap_t
cap_pdpt_new (uintptr_t pdpt_phy)
{
  cap_t cap = { .type = cap_x86_64_pdpt };
  cap_set_ptr (&cap, (void *)pdpt_phy);
  return cap;
}

static inline cap_t
cap_pd_new (uintptr_t pd_phy)
{
  cap_t cap = { .type = cap_x86_64_pd };
  cap_set_ptr (&cap, (void *)pd_phy);
  return cap;
}

static inline cap_t
cap_pt_new (uintptr_t pt_phy)
{
  cap_t cap = { .type = cap_x86_64_pt };
  cap_set_ptr (&cap, (void *)pt_phy);
  return cap;
}

static inline cap_t
cap_page_new (uintptr_t frame_phy)
{
  cap_t cap = { .type = cap_x86_64_page };
  cap_set_ptr (&cap, (void *)frame_phy);
  return cap;
}

static inline exception_t
lookup_cap (cap_t cspace_root, word_t index, word_t depth, cap_t *cap)
{
  assert (depth == 64); // for now
  assert (cap_type (cspace_root) == cap_cnode);

  cte_t *cte = cap_ptr (cspace_root);
  size_t length = cap_size (cspace_root);
  assert (index < length);

  *cap = cte[index].cap;
  return no_error;
}

static inline cap_t *
lookup_cap_ref (cap_t cspace_root, word_t index, word_t depth, error_t *err)
{
  assert (depth == 64); // for now
  assert (cap_type (cspace_root) == cap_cnode);

  cte_t *cte = cap_ptr (cspace_root);
  size_t length = cap_size (cspace_root);
  assert (index < length);

  *err = no_error;
  return &cte[index].cap;
}

static inline cte_t *
lookup_cap_slot (cap_t cspace_root, word_t index, word_t depth, error_t *err)
{
  assert (depth == 64); // for now
  assert (cap_type (cspace_root) == cap_cnode);

  cte_t *cte = cap_ptr (cspace_root);
  size_t length = cap_size (cspace_root);
  assert (index < length);

  *err = no_error;
  return &cte[index];
}

static inline word_t
cte_ptr_type (cte_t *cte)
{
  return cap_type (cte->cap);
}

static inline void *
cte_ptr (cte_t *cte)
{
  return cap_ptr (cte->cap);
}

static inline void
cte_set_ptr (cte_t *cte, void *ptr)
{
  cap_set_ptr (&cte->cap, ptr);
}

static inline word_t
cte_size (cte_t *cte)
{
  return cap_size (cte->cap);
}

static inline void
cte_set_size (cte_t *cte, word_t size)
{
  cte->cap.size_bits = size;
}

static inline word_t
cte_rights (cte_t *cte)
{
  return cte->cap.rights;
}

static inline void
cte_set_rights (cte_t *cte, word_t rights)
{
  cte->cap.rights = rights;
}

#define cap_type(c) _Generic ((c), cap_t: cap_type, cte_t *: cte_ptr_type) (c)
#define cap_ptr(c) _Generic ((c), cap_t: cap_ptr, cte_t *: cte_ptr) (c)
#define cap_size(c) _Generic ((c), cap_t: cap_size, cte_t *: cte_size) (c)
#define cap_rights(c)                                                         \
  _Generic ((c), cap_t: cap_rights, cte_t *: cte_rights) (c)
#define cap_set_ptr(c, p)                                                     \
  _Generic ((c), cap_t: cap_set_ptr, cte_t *: cte_set_ptr) (c, p)
#define cap_set_size(c, s)                                                    \
  _Generic ((c), cap_t: cap_set_size, cte_t *: cte_set_size) (c, s)
#define cap_set_rights(c, r)                                                  \
  _Generic ((c), cap_t: cap_set_rights, cte_t *: cte_set_rights) (c, r)

static inline const char *
cte_type_string (cte_t *cte)
{
  return cap_type_string (cte_ptr_type (cte));
}

static inline const char *
cap_value_type_string (cap_t cap)
{
  return cap_type_string (cap_type (cap));
}

#define cap_type_string(t)                                                    \
  _Generic ((t), word_t: cap_type_string, cap_t: cap_value_type_string, cte_t *: cte_type_string) (t)
