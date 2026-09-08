#pragma once

#include "assert.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

constexpr uintptr_t BITS_PTR_MASK = 0xFFFF 'FFFF' FFF0;

static inline bool
is_safe_cap_ptr (void *ptr)
{
  const uintptr_t p = (uintptr_t)ptr;
  const uintptr_t low_bits = p & 0xF;
  if (low_bits != 0)
    return false;
  const uintptr_t high_bits = p >> 47;
  if (high_bits != 0x1FFFF && high_bits != 0)
    return false;
  return true;
}

static inline void *
bits_pointer (word_t bits)
{
  const word_t ptr_bits = bits & BITS_PTR_MASK;
  if (ptr_bits & 0x8000 '0000' 0000)
    return (void *)(ptr_bits | 0xFFFF'0000'0000'0000);
  else
    return (void *)ptr_bits;
}

static inline void
set_bits_pointer (word_t *bits, void *ptr)
{
  assert (is_safe_cap_ptr (ptr));
  *bits &= ~BITS_PTR_MASK;
  *bits |= (word_t)ptr & BITS_PTR_MASK;
}

union capability
{
  word_t words[2];
  struct
  {
    word_t is_original : 1;
    word_t is_device : 1;
    word_t reserved : 2;
    word_t ptr : 44;
    word_t size_bits : 6;
    word_t rights : 5;
    word_t type : 5;
    word_t badge;
  };
};

typedef union capability cap_t;

static_assert (sizeof (cap_t) == 16, "cap_t size is not 16 bytes");

struct cte
{
  cap_t cap;
  struct cte *next, *prev;
};

typedef struct cte cte_t;

static_assert (sizeof (cte_t) == 32, "cte_t size is not 32 bytes");

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
cap_set_size_bits (cap_t *cap, word_t size_bits)
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

static inline cap_t
new_null_cap ()
{
  cap_t cap = { .type = CAP_NULL };
  return cap;
}

static inline cap_t
new_untyped_cap (uintptr_t paddr, uintptr_t size_bits)
{
  cap_t cap = { .type = CAP_UNTYPED, .size_bits = size_bits };
  cap_set_ptr (&cap, (void *)paddr);
  return cap;
}

static inline cap_t
new_untyped_device_cap (uintptr_t paddr, uintptr_t size_bits)
{
  cap_t cap = {
    .type = CAP_UNTYPED,
    .size_bits = size_bits,
    .is_device = 1,
  };
  cap_set_ptr (&cap, (void *)paddr);
  return cap;
}

static inline cap_t
new_endpoint_cap (void *endpoint, uintptr_t badge)
{
  cap_t cap = { .type = CAP_ENDPOINT, .badge = badge };
  cap_set_ptr (&cap, endpoint);
  return cap;
}

static inline cap_t
new_cnode_cap (void *cnode, uintptr_t size_bits)
{
  cap_t cap = { .type = CAP_CNODE, .size_bits = size_bits };
  cap_set_ptr (&cap, cnode);
  return cap;
}

static inline cap_t
new_tcb_cap (void *tcb)
{
  cap_t cap = { .type = CAP_TCB };
  cap_set_ptr (&cap, tcb);
  return cap;
}

static inline cap_t
new_x86_64_io_port_control_cap ()
{
  cap_t cap = { .type = CAP_X86_64_IO_PORT_CONTROL };
  return cap;
}

static inline cap_t
new_x86_64_io_port_cap (uint16_t first_port, uint16_t last_port)
{
  cap_t cap
      = { .type = CAP_X86_64_IO_PORT, .ptr = first_port, .badge = last_port };
  return cap;
}

static inline uint16_t
cap_x86_64_io_port_first_port (cap_t cap)
{
  return cap.ptr;
}

static inline uint16_t
cap_x86_64_io_port_last_port (cap_t cap)
{
  return cap.badge;
}

static inline cap_t
new_x86_64_pml4_cap (uintptr_t pml4_phy)
{
  cap_t cap = { .type = CAP_X86_64_PML4 };
  cap_set_ptr (&cap, (void *)pml4_phy);
  return cap;
}

static inline cap_t
new_x86_64_pdpt_cap (uintptr_t pdpt_phy)
{
  cap_t cap = { .type = CAP_X86_64_PDPT };
  cap_set_ptr (&cap, (void *)pdpt_phy);
  return cap;
}

static inline cap_t
new_x86_64_pd_cap (uintptr_t pd_phy)
{
  cap_t cap = { .type = CAP_X86_64_PD };
  cap_set_ptr (&cap, (void *)pd_phy);
  return cap;
}

static inline cap_t
new_x86_64_pt_cap (uintptr_t pt_phy)
{
  cap_t cap = { .type = CAP_X86_64_PT };
  cap_set_ptr (&cap, (void *)pt_phy);
  return cap;
}

static inline cap_t
new_x86_64_page_cap (uintptr_t frame_phy)
{
  cap_t cap = { .type = CAP_X86_64_PAGE };
  cap_set_ptr (&cap, (void *)frame_phy);
  return cap;
}

static inline cap_t
new_irq_control_cap ()
{
  return (cap_t){ .type = CAP_IRQ_CONTROL };
}

static inline cap_t
new_irq_handler_cap (word_t irq)
{
  cap_t cap = { .type = CAP_IRQ_HANDLER, .size_bits = irq };
  return cap;
}

// Low-level lookup that returns error_t without touching IPC buffer
error_t lookup_cap_slot_raw (cte_t *cspace_root, word_t index, word_t depth,
                             cte_t **out);

// High-level lookup that formats errors into IPC buffer
message_info_t lookup_cap_slot (cte_t *cspace_root, word_t index, word_t depth,
                                cte_t **out);

#define lookup_cap_slot_this_tcb(index, out)                                  \
  lookup_cap_slot (&this_tcb->cspace_root, index, 64, out)

static inline word_t
cte_ptr_type (const cte_t *cte)
{
  return cap_type (cte->cap);
}

static inline void *
cte_ptr (const cte_t *cte)
{
  return cap_ptr (cte->cap);
}

static inline void
cte_set_ptr (cte_t *cte, void *ptr)
{
  cap_set_ptr (&cte->cap, ptr);
}

static inline word_t
cte_size (const cte_t *cte)
{
  return cap_size (cte->cap);
}

static inline void
cte_set_size_bits (cte_t *cte, word_t size_bits)
{
  cte->cap.size_bits = size_bits;
}

static inline word_t
cte_rights (const cte_t *cte)
{
  return cte->cap.rights;
}

static inline void
cte_set_rights (cte_t *cte, word_t rights)
{
  cte->cap.rights = rights;
}

static inline const char *
cte_type_string (const cte_t *cte)
{
  return cap_type_string (cte_ptr_type (cte));
}

static inline const char *
cap_value_type_string (cap_t cap)
{
  return cap_type_string (cap_type (cap));
}

#define cap_type(c)                                                           \
  _Generic ((c),                                                              \
      cap_t: cap_type,                                                        \
      cte_t *: cte_ptr_type,                                                  \
      const cte_t *: cte_ptr_type) (c)
#define cap_ptr(c)                                                            \
  _Generic ((c), cap_t: cap_ptr, cte_t *: cte_ptr, const cte_t *: cte_ptr) (c)
#define cap_size(c)                                                           \
  _Generic ((c),                                                              \
      cap_t: cap_size,                                                        \
      cte_t *: cte_size,                                                      \
      const cte_t *: cte_size) (c)
#define cap_rights(c)                                                         \
  _Generic ((c),                                                              \
      cap_t: cap_rights,                                                      \
      cte_t *: cte_rights,                                                    \
      const cte_t *: cte_rights) (c)
#define cap_type_string(t)                                                    \
  _Generic ((t),                                                              \
      word_t: cap_type_string,                                                \
      cap_t: cap_value_type_string,                                           \
      cte_t *: cte_type_string,                                               \
      const cte_t *: cte_type_string) (t)
#define cap_set_ptr(c, p)                                                     \
  _Generic ((c), cap_t: cap_set_ptr, cte_t *: cte_set_ptr) (c, p)
#define cap_set_size_bits(c, s)                                               \
  _Generic ((c), cap_t *: cap_set_size_bits, cte_t *: cte_set_size_bits) (c, s)
#define cap_set_rights(c, r)                                                  \
  _Generic ((c), cap_t: cap_set_rights, cte_t *: cte_set_rights) (c, r)

void insert_cte_after (struct cte *new, struct cte *after);
void unlink_cte (struct cte *del);
message_info_t copy_cap (struct cte *dest, struct cte *src, cap_rights_t);
message_info_t mint_cap (struct cte *dest, struct cte *src, word_t badge,
                         cap_rights_t);
bool is_child_cap (const struct cte *c, const struct cte *parent);
message_info_t delete_cap (struct cte *c);
message_info_t revoke_cap (struct cte *c);
