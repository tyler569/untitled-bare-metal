#include "asm.h"
#include "stdio.h"
#include "string.h"
#include "sys/cdefs.h"
#include "x86_64.h"

static_assert (offsetof (per_cpu_t, tss.rsp[2]) == TSS_RSP2,
               "tss rsp2 offset needs to be changed in asm.h");
static_assert (offsetof (per_cpu_t, kernel_stack_top) == TSS_STACK,
               "tss ist0 offset needs to be changed in asm.h");

#define KERNEL_CODE 0x9A
#define KERNEL_DATA 0x92
#define USER_CODE 0xFA
#define USER_DATA 0xF2
#define TSS 0x89
#define LONG_MODE 0x20

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

per_cpu_t bsp_cpu = {
  .self = &bsp_cpu,
  .gdt = {
    DESCRIPTOR (0, 0),
    DESCRIPTOR (KERNEL_CODE, LONG_MODE),
    DESCRIPTOR (KERNEL_DATA, LONG_MODE),
    DESCRIPTOR (USER_DATA, LONG_MODE),
    DESCRIPTOR (USER_CODE, LONG_MODE),
    TSS_DESCRIPTOR_LOW (),
    TSS_DESCRIPTOR_HIGH (),
  },
};

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
init_fsgs ()
{
  uintptr_t cr4 = read_cr4 ();
  write_cr4 (cr4 | CR4_FSGSBASE);
}

void
init_gdt (per_cpu_t *cpu)
{
  cpu->gdt[5].base_low = (uintptr_t)&cpu->tss;
  cpu->gdt[5].base_middle = (uintptr_t)&cpu->tss >> 16;
  cpu->gdt[5].base_high = (uintptr_t)&cpu->tss >> 24;
  cpu->gdt[6].base_upper = (uintptr_t)&cpu->tss >> 32;

  cpu->gdt_ptr.limit = sizeof (cpu->gdt) - 1;
  cpu->gdt_ptr.base = (uintptr_t)cpu->gdt;

  load_gdt (&cpu->gdt_ptr);

  jump_to_gdt ();
  reset_segment_registers ();

  init_fsgs ();
  write_gsbase ((uintptr_t)cpu);
}

void
init_bsp_gdt ()
{
  init_gdt (&bsp_cpu);
}

void
init_ap_gdt (per_cpu_t *cpu)
{
  memcpy (cpu->gdt, bsp_cpu.gdt, sizeof (bsp_cpu.gdt));
  memset (&cpu->tss, 0, sizeof (tss_t));

  init_gdt (cpu);
}