#pragma once

#include "list.h"
#include "stddef.h"
#include "stdint.h"

#define PAGE_SIZE 4096

#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_CSTAR 0xC0000083
#define IA32_FMASK 0xC0000084
#define IA32_FS_BASE 0xC0000100
#define IA32_GS_BASE 0xC0000101

#define IA32_EFER_SCE 0x1

#define CR4_FSGSBASE 0x100

#define KERNEL_CS 0x08l
#define USER_CS 0x23l
// In long mode, SYSRET pulls its code segment from IA32_STAR 63:48 + 16
// and its stack segment from IA32_STAR 63:48 + 8. This is the fake code
// segment that we load into IA32_STAR, so that the real code segment is
// KERNEL_CS.
#define USER_FAKE_SYSRET_CS (USER_CS - 16)

#define FLAG_CF 0x0001
#define FLAG_PF 0x0004
#define FLAG_AF 0x0010
#define FLAG_ZF 0x0040
#define FLAG_SF 0x0080
#define FLAG_TF 0x0100
#define FLAG_IF 0x0200
#define FLAG_DF 0x0400
#define FLAG_OF 0x0800

#define PF_PRESENT 0x1
#define PF_WRITE 0x2
#define PF_USER 0x4
#define PF_RESERVED 0x8
#define PF_EXECUTE 0x10
#define PF_PROTECTION_KEY 0x20
#define PF_SHADOW_STACK 0x40

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

struct per_cpu
{
  struct per_cpu *self;
  tss_t tss;
  gdt_entry_t gdt[7];
  gdt_ptr_t gdt_ptr;
  uintptr_t kernel_stack_top;
  struct list_head list;
};

typedef struct per_cpu per_cpu_t;

#define this_cpu ((__seg_gs per_cpu_t *)0)

void init_bsp_gdt ();
void init_ap_gdt (per_cpu_t *cpu);
void init_ap_idt ();
void init_idt ();
void init_aps ();
void init_syscall ();
void init_int_stacks ();

void write_port_b (uint16_t port, uint8_t);
uint8_t read_port_b (uint16_t port);
void write_port_w (uint16_t port, uint16_t);
uint16_t read_port_w (uint16_t port);
void write_port_l (uint16_t port, uint32_t);
uint32_t read_port_l (uint16_t port);

void write_msr (uint32_t msr_id, uint64_t);
uint64_t read_msr (uint32_t msr_id);

void write_fsbase (uintptr_t);
void write_gsbase (uintptr_t);

uint64_t read_cr2 ();
uint64_t read_cr4 ();
void write_cr4 (uint64_t);

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct frame frame_t;

struct syscall_frame
{
  uint64_t r15, r14, r13, r12, rbp, rbx;
  uint64_t rflags, rip, rsp;
};

void print_interrupt_info (frame_t *);

// interrupt service routine prototypes

void isr0 ();
void isr1 ();
void isr2 ();
void isr3 ();
void isr4 ();
void isr5 ();
void isr6 ();
void isr7 ();
void isr8 ();
void isr9 ();
void isr10 ();
void isr11 ();
void isr12 ();
void isr13 ();
void isr14 ();
void isr15 ();
void isr16 ();
void isr17 ();
void isr18 ();
void isr19 ();
void isr20 ();
void isr21 ();
void isr22 ();
void isr23 ();
void isr24 ();
void isr25 ();
void isr26 ();
void isr27 ();
void isr28 ();
void isr29 ();
void isr30 ();
void isr31 ();
void irq0 ();
void irq1 ();
void irq2 ();
void irq3 ();
void irq4 ();
void irq5 ();
void irq6 ();
void irq7 ();
void irq8 ();
void irq9 ();
void irq10 ();
void irq11 ();
void irq12 ();
void irq13 ();
void irq14 ();
void irq15 ();

void syscall_entry ();
