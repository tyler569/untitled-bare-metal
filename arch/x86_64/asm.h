#pragma once

// offsets into per_cpu_t, see static assertions in gdt.c
#define TSS_RSP2 28
#define TSS_STACK 184

// clang-format off
#ifdef __ASSEMBLER__

.macro PUSH_ALL
    push %rax
    push %rcx
    push %rdx
    push %rbx
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
.endm

.macro POP_ALL
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax
.endm

.macro PUSH_CALLEE_SAVED
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
.endm

.macro POP_CALLEE_SAVED
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
.endm

#endif