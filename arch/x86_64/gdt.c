#include "string.h"
#include "sys/cdefs.h"
#include "x86_64.h"

#define KERNEL_CODE 0x9A
#define KERNEL_DATA 0x92
#define USER_CODE 0xFA
#define USER_DATA 0xF2
#define TSS 0x89
#define LONG_MODE 0x20

#define DESCRIPTOR(a, g) { .access = a, .granularity = g }
#define TSS_DESCRIPTOR_LOW() { .access = TSS, .limit_low = sizeof (tss_t) - 1 }
#define TSS_DESCRIPTOR_HIGH()                                                 \
  {                                                                           \
  }

per_cpu_t bsp_cpu = {
  .self = &bsp_cpu,
  .arch = {
    .gdt = {
      DESCRIPTOR (0, 0),
      DESCRIPTOR (KERNEL_CODE, LONG_MODE),
      DESCRIPTOR (KERNEL_DATA, LONG_MODE),
      DESCRIPTOR (USER_DATA, LONG_MODE),
      DESCRIPTOR (USER_CODE, LONG_MODE),
      TSS_DESCRIPTOR_LOW (),
      TSS_DESCRIPTOR_HIGH (),
    },
    .tss = {
      .iomap_base = sizeof (tss_t),
    },
  },
};

void
load_gdt (gdt_ptr_t *g)
{
  asm volatile ("lgdt %0" : : "m"(*g));
  asm volatile ("ltr %w0" : : "r"(0x28));
}

struct PACKED long_jump
{
  void *target;
  uint16_t segment;
};

void
jump_to_gdt ()
{
  asm volatile ("push $0x08\n\t"
                "push $1f\n\t"
                "lretq\n\t"
                "1:\n\t");
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
set_tls_base (uintptr_t base)
{
  write_msr (IA32_FS_BASE, base);
}

void
init_gdt (per_cpu_t *cpu)
{
  cpu->arch.gdt[5].base_low = (uintptr_t)&cpu->arch.tss;
  cpu->arch.gdt[5].base_middle = (uintptr_t)&cpu->arch.tss >> 16;
  cpu->arch.gdt[5].base_high = (uintptr_t)&cpu->arch.tss >> 24;
  cpu->arch.gdt[6].base_upper = (uintptr_t)&cpu->arch.tss >> 32;

  struct gdt_ptr ptr = {
    .limit = sizeof (cpu->arch.gdt) - 1,
    .base = (uintptr_t)cpu->arch.gdt,
  };

  load_gdt (&ptr);

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
  memcpy (cpu->arch.gdt, bsp_cpu.arch.gdt, sizeof (bsp_cpu.arch.gdt));
  memset (&cpu->arch.tss, 0, sizeof (tss_t));

  init_gdt (cpu);
}
