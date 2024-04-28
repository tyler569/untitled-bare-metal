#include "asm.h"

.global jump_to_userland
jump_to_userland:
    xchg %rsi, %rsp
    mov %rdi, %rcx
    mov $0, %r11
    ; swapgs
    sysretq

.section .bss
BAD_syscall_stack:
    .fill 0x1000, 1, 0
.equ BAD_kernel_stack, BAD_syscall_stack + 0x1000
.section .text

.global syscall_entry
syscall_entry:
    ; swapgs
    lea BAD_kernel_stack, %rbx
    xchg %rbx, %rsp
    push %rbx
    push %rcx
    push %r11
    PUSH_CALLEE_SAVED
    push %rax    // 7th paramter to the function
    call c_syscall_entry
    add $8, %rsp // caller pops 7th parameter
    POP_CALLEE_SAVED
    pop %r11
    pop %rcx
    pop %rsp
    ; swapgs
    sysretq
