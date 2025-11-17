#include "asm.h"

.global jump_to_userland
jump_to_userland:
    xor %r11, %r11
    mov %rdi, %rcx
    mov %rsi, %rsp

    swapgs
    sysretq

.global syscall_entry
syscall_entry:
    swapgs

    mov %rsp, %gs:(TSS_RSP2)
    mov %gs:(TSS_STACK), %rsp

    // match the stack layout of the processors interrupt handler,
    // so we can use the same frame_t.
    push $USER_SS       // ss
    push %gs:(TSS_RSP2) // rsp
    push %r11           // user rflags
    push $USER_CS       // cs
    push %rcx           // user rip
    push $0             // error code
    push $0             // int number
    PUSH_ALL            // save registers
    mov %rax, %rdx      // syscall_number is 3rd parameter
    mov %rsp, %rcx      // stack pointer is frame_t * is 4th parameter
    call c_syscall_entry
    POP_ALL             // restore registers
    add $16, %rsp       // pop error code, int number
    pop %rcx            // user rip
    add $8, %rsp        // pop cs
    pop %r11            // pop rflags
    pop %rsp
    // pushed ss gets discarded under the new stack pointer

    swapgs
    sysretq
