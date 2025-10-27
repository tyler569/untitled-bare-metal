#pragma once

#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

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

// Unified capability structure: merges capability data + derivation tree
struct cap
{
  // Capability data (16 bytes) - bitfield encoded
  union
  {
    word_t words[2];
    struct
    {
      word_t is_original : 1;
      word_t is_device : 1;
      word_t ptr : 46;
      word_t size_bits : 6;
      word_t rights : 5;
      word_t type : 5;
      word_t badge;
    };
  };

  // Capability derivation tree (16 bytes)
  struct cap *prev;
  struct cap *next;
};

static_assert (sizeof (struct cap) == 32, "struct cap size is not 32 bytes");

// Capability accessors
static inline void *
cap_ptr (struct cap *cap)
{
  return bits_pointer (cap->words[0]);
}

static inline void
cap_set_ptr (struct cap *cap, void *ptr)
{
  set_bits_pointer (&cap->words[0], ptr);
}

static inline word_t
cap_type (struct cap *cap)
{
  return cap->type;
}

static inline word_t
cap_size (struct cap *cap)
{
  return BIT (cap->size_bits);
}

static inline void
cap_set_size (struct cap *cap, word_t size_bits)
{
  cap->size_bits = size_bits;
}

static inline word_t
cap_rights (struct cap *cap)
{
  return cap->rights;
}

static inline void
cap_set_rights (struct cap *cap, word_t rights)
{
  cap->rights = rights;
}

// Capability derivation tree accessors
static inline struct cap *
cap_prev (struct cap *cap)
{
  return cap->prev;
}

static inline struct cap *
cap_next (struct cap *cap)
{
  return cap->next;
}

static inline void
cap_set_prev (struct cap *cap, struct cap *prev)
{
  cap->prev = prev;
}

static inline void
cap_set_next (struct cap *cap, struct cap *next)
{
  cap->next = next;
}

// Capability initializers
static inline void
cap_null_init (struct cap *cap)
{
  *cap = (struct cap){ .type = cap_null };
}

static inline void
cap_untyped_init (struct cap *cap, uintptr_t paddr, uintptr_t size_bits)
{
  *cap = (struct cap){ .type = cap_untyped,
                       .size_bits = size_bits,
                       .is_original = 1 };
  cap_set_ptr (cap, (void *)paddr);
}

static inline void
cap_untyped_device_init (struct cap *cap, uintptr_t paddr, uintptr_t size_bits)
{
  *cap = (struct cap){
    .type = cap_untyped,
    .size_bits = size_bits,
    .is_device = 1,
    .is_original = 1,
  };
  cap_set_ptr (cap, (void *)paddr);
}

static inline void
cap_endpoint_init (struct cap *cap, void *endpoint, uintptr_t badge)
{
  *cap
      = (struct cap){ .type = cap_endpoint, .badge = badge, .is_original = 1 };
  cap_set_ptr (cap, endpoint);
}

static inline void
cap_cnode_init (struct cap *cap, void *cnode, uintptr_t size_bits)
{
  *cap = (struct cap){ .type = cap_cnode,
                       .size_bits = size_bits,
                       .is_original = 1 };
  cap_set_ptr (cap, cnode);
}

static inline void
cap_tcb_init (struct cap *cap, void *tcb)
{
  *cap = (struct cap){ .type = cap_tcb, .is_original = 1 };
  cap_set_ptr (cap, tcb);
}

static inline void
cap_x86_64_io_port_control_init (struct cap *cap)
{
  *cap = (struct cap){ .type = cap_x86_64_io_port_control, .is_original = 1 };
}

static inline void
cap_x86_64_io_port_init (struct cap *cap, uint16_t first_port,
                         uint16_t last_port)
{
  *cap = (struct cap){
    .type = cap_x86_64_io_port,
    .ptr = first_port,
    .badge = last_port,
    .is_original = 1,
  };
}

static inline uint16_t
cap_x86_64_io_port_first_port (struct cap *cap)
{
  return cap->ptr;
}

static inline uint16_t
cap_x86_64_io_port_last_port (struct cap *cap)
{
  return cap->badge;
}

static inline void
cap_x86_64_pml4_init (struct cap *cap, uintptr_t pml4_phy)
{
  *cap = (struct cap){ .type = cap_x86_64_pml4, .is_original = 1 };
  cap_set_ptr (cap, (void *)pml4_phy);
}

static inline void
cap_x86_64_pdpt_init (struct cap *cap, uintptr_t pdpt_phy)
{
  *cap = (struct cap){ .type = cap_x86_64_pdpt, .is_original = 1 };
  cap_set_ptr (cap, (void *)pdpt_phy);
}

static inline void
cap_x86_64_pd_init (struct cap *cap, uintptr_t pd_phy)
{
  *cap = (struct cap){ .type = cap_x86_64_pd, .is_original = 1 };
  cap_set_ptr (cap, (void *)pd_phy);
}

static inline void
cap_x86_64_pt_init (struct cap *cap, uintptr_t pt_phy)
{
  *cap = (struct cap){ .type = cap_x86_64_pt, .is_original = 1 };
  cap_set_ptr (cap, (void *)pt_phy);
}

static inline void
cap_x86_64_page_init (struct cap *cap, uintptr_t frame_phy)
{
  *cap = (struct cap){ .type = cap_x86_64_page, .is_original = 1 };
  cap_set_ptr (cap, (void *)frame_phy);
}

static inline void
cap_irq_control_init (struct cap *cap)
{
  *cap = (struct cap){ .type = cap_irq_control, .is_original = 1 };
}

static inline void
cap_irq_handler_init (struct cap *cap, word_t irq)
{
  *cap = (struct cap){ .type = cap_irq_handler,
                       .size_bits = irq,
                       .is_original = 1 };
}

// Low-level lookup that returns error_t without touching IPC buffer
error_t lookup_cap_slot_raw (struct cap *cspace_root, word_t index,
                             word_t depth, struct cap **out);

// High-level lookup that formats errors into IPC buffer
message_info_t lookup_cap_slot (struct cap *cspace_root, word_t index,
                                word_t depth, struct cap **out);

#define lookup_cap_slot_this_tcb(index, out)                                  \
  lookup_cap_slot (&this_tcb->cspace_root, index, 64, out)

// Capability type string helper
static inline const char *
cap_type_string_from_cap (struct cap *cap)
{
  return cap_type_string (cap_type (cap));
}

// Copy capability data (without CDT manipulation)
static inline void
copy_cap_data (struct cap *dest, struct cap *src, cap_rights_t rights)
{
  // Copy the capability bitfield data (first 16 bytes)
  dest->words[0] = src->words[0];
  dest->words[1] = src->words[1];
  // Restrict rights
  dest->rights &= rights;
  // Note: prev/next are NOT copied - CDT linkage is separate
  // Note: is_original IS copied - caller must set it explicitly afterward
}

// Capability Derivation Tree operations
void cdt_insert (struct cap *new, struct cap *parent, struct cap *next);
void cdt_remove (struct cap *cap);
bool is_child (struct cap *child, struct cap *parent);
error_t cap_delete (struct cap *cap);
error_t cap_revoke (struct cap *cap);
