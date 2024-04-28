#pragma once

#include "stdint.h"

#define PAGE_SIZE 4096

#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_CSTAR 0xC0000083
#define MSR_SYSCALL_FLAG_MASK 0xC0000084

#define IA32_EFER 0xC0000080
#define IA32_EFER_SCE 0x1

#define KERNEL_CS 0x08l
#define USER_CS 0x23l
// In long mode, SYSRET pulls its code segment from MSR_STAR 63:48 + 16
// and its stack segment from MSR_STAR 63:48 + 8. This is the fake code
// segment that we load into MSR_STAR, so that the real code segment is
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

void init_gdt ();
void init_idt ();
void init_aps ();
void init_syscall ();

void write_port_b (uint16_t port, uint8_t value);
uint8_t read_port_b (uint16_t port);

void write_msr (uint32_t msr_id, uint64_t value);
uint64_t read_msr (uint32_t msr_id);

uintptr_t read_cr2 ();

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct frame frame_t;

void print_interrupt_info (frame_t *f);

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

#define SYSCALL_ARGS                                                          \
  uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8,        \
      uint64_t r9

void syscall_entry (SYSCALL_ARGS);