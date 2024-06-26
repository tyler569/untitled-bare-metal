#pragma once

#include "stddef.h"
#include "stdint.h"
#include "sys/cdefs.h"

#define PAGE_SIZE 4096ul

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

struct arch_per_cpu
{
  tss_t tss;
  gdt_entry_t gdt[7];
};

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct frame frame_t;

#define this_cpu ((__seg_gs per_cpu_t *)0)
