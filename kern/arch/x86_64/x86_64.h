#pragma once

// Segment selectors shared with syscall.S.
#define USER_CS 0x23UL
#define USER_SS 0x1BUL

#ifndef __ASSEMBLER__

#include "kern/arch/x86_64/exports.h"
#include "kern/per_cpu.h"
#include "stddef.h"
#include "stdint.h"

constexpr uint32_t IA32_EFER = 0xC0000080;
constexpr uint32_t IA32_STAR = 0xC0000081;
constexpr uint32_t IA32_LSTAR = 0xC0000082;
constexpr uint32_t IA32_CSTAR = 0xC0000083;
constexpr uint32_t IA32_FMASK = 0xC0000084;
constexpr uint32_t IA32_FS_BASE = 0xC0000100;
constexpr uint32_t IA32_GS_BASE = 0xC0000101;

constexpr uint64_t IA32_EFER_SCE = 1;

constexpr uint64_t CR0_MP = 1 << 1;
constexpr uint64_t CR0_EM = 1 << 2;
constexpr uint64_t CR4_OSFXSR = 1 << 9;
constexpr uint64_t CR4_OSXMMEXCPT = 1 << 10;
constexpr uint64_t CR4_FSGSBASE = 1 << 16;

constexpr uint64_t KERNEL_CS = 0x08;
constexpr uint64_t KERNEL_SS = 0x10;

// In long mode, SYSRET pulls its code segment from IA32_STAR 63:48 + 16
// and its stack segment from IA32_STAR 63:48 + 8. This is the fake code
// segment that we load into IA32_STAR, so that the real code segment is
// KERNEL_CS.
constexpr uint64_t USER_FAKE_SYSRET_CS = USER_CS - 16;

constexpr uint64_t FLAG_CF = 0x1;
constexpr uint64_t FLAG_PF = 0x4;
constexpr uint64_t FLAG_AF = 0x10;
constexpr uint64_t FLAG_ZF = 0x40;
constexpr uint64_t FLAG_SF = 0x80;
constexpr uint64_t FLAG_TF = 0x100;
constexpr uint64_t FLAG_IF = 0x200;
constexpr uint64_t FLAG_DF = 0x400;
constexpr uint64_t FLAG_OF = 0x800;

constexpr uint8_t IST_NMI = 1;
constexpr uint8_t IST_DF = 2;

#endif // __ASSEMBLER__

// Offsets shared with syscall.S.
#define TSS_RSP2 28UL
#define TSS_STACK 168UL

#ifndef __ASSEMBLER__

static_assert (offsetof (per_cpu_t, arch.tss.rsp[2]) == TSS_RSP2,
               "tss rsp2 offset needs to be changed in asm.h");
static_assert (offsetof (per_cpu_t, kernel_stack_top) == TSS_STACK,
               "offset needs to be changed in asm.h");

void init_bsp_gdt ();
void init_ap_gdt (per_cpu_t *cpu);
void init_ap_idt ();
void init_idt ();
void init_aps ();
void init_sse ();
void init_syscall ();
void init_int_stacks ();
void init_ap_int_stacks ();
void init_pic ();
void init_ioapic ();
void init_lapic ();

void send_eoi (uint8_t irq);

extern per_cpu_t bsp_cpu;

typedef uint64_t pte_t;

pte_t *get_pml4e (uintptr_t root, uintptr_t addr);
pte_t *get_pdpte (uintptr_t root, uintptr_t addr);
pte_t *get_pde (uintptr_t root, uintptr_t addr);
pte_t *get_pte (uintptr_t root, uintptr_t addr);

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

uint64_t read_cr0 ();
void write_cr0 (uint64_t);
uint64_t read_cr2 ();
uint64_t read_cr4 ();
void write_cr4 (uint64_t);

void save_frame_on_tcb (frame_t *);
void clear_frame_on_tcb (frame_t *);

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
void isr255 ();

void syscall_entry ();

#endif // __ASSEMBLER__
