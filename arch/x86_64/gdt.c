#include "stdio.h"
#include "sys/cdefs.h"
#include "x86_64.h"

#define KERNEL_CODE 0x9A
#define KERNEL_DATA 0x92
#define USER_CODE 0xFA
#define USER_DATA 0xF2
#define TSS 0x89
#define LONG_MODE 0x20

union PACKED gdt_entry
{
  struct
  {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
  };
  uint32_t base_upper;
};

struct PACKED gdt_ptr
{
  uint16_t limit;
  uintptr_t base;
};

struct PACKED tss
{
  uint32_t reserved;
  uint64_t rsp[3];
  uint64_t reserved2;
  uint64_t ist[7];
  uint64_t reserved3;
  uint16_t reserved4;
  uint16_t iomap_base;
};

typedef union gdt_entry gdt_entry_t;
typedef struct gdt_ptr gdt_ptr_t;
typedef struct tss tss_t;

#define DESCRIPTOR(a, g)                                                      \
  {                                                                           \
    .access = a, .granularity = g                                             \
  }
#define TSS_DESCRIPTOR_LOW()                                                  \
  {                                                                           \
    .access = TSS, .limit_low = sizeof (tss_t) - 1                            \
  }
#define TSS_DESCRIPTOR_HIGH()                                                 \
  {                                                                           \
  }

static tss_t tss = { 0 };

static gdt_entry_t gdt[] = {
  DESCRIPTOR (0, 0),
  DESCRIPTOR (KERNEL_CODE, LONG_MODE),
  DESCRIPTOR (KERNEL_DATA, LONG_MODE),
  DESCRIPTOR (USER_DATA, LONG_MODE),
  DESCRIPTOR (USER_CODE, LONG_MODE),
  TSS_DESCRIPTOR_LOW (),
  TSS_DESCRIPTOR_HIGH (),
};

static gdt_ptr_t gdt_ptr = { sizeof (gdt) - 1, (uintptr_t)gdt };

void
load_gdt (gdt_ptr_t *g)
{
  asm volatile ("lgdt %0" : : "m"(*g));
}

struct PACKED long_jump
{
  void *target;
  uint16_t segment;
};

void
jump_to_gdt ()
{
  struct long_jump lj = { &&target, 8 };

  asm volatile ("ljmpq *(%%rax)" : : "a"(&lj));
target:
}

void
reset_segment_registers ()
{
  asm volatile ("mov %0, %%ds" : : "r"(0));
  asm volatile ("mov %0, %%es" : : "r"(0));
  asm volatile ("mov %0, %%fs" : : "r"(0));
  asm volatile ("mov %0, %%gs" : : "r"(0));
  asm volatile ("mov %0, %%ss" : : "r"(0));
}

void
init_gdt ()
{
  gdt[5].base_low = (uintptr_t)&tss;
  gdt[5].base_middle = (uintptr_t)&tss >> 16;
  gdt[5].base_high = (uintptr_t)&tss >> 24;
  gdt[6].base_upper = (uintptr_t)&tss >> 32;

  load_gdt (&gdt_ptr);

  jump_to_gdt ();
  reset_segment_registers ();
}
