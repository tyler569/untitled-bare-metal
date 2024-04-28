#include "asm.h"

.global jump_to_userland
jump_to_userland:
    xchg %rsi, %rsp
    mov %rdi, %rcx
    mov $0, %r11

    swapgs
    sysretq

.global syscall_entry
syscall_entry:
    swapgs

    mov %rsp, %gs:(TSS_RSP2)
    mov %gs:(TSS_STACK), %rsp
    push %gs:(TSS_RSP2)

    push %rcx           // user rip
    push %r11           // user rflags
    PUSH_CALLEE_SAVED   // save callee saved registers
    mov %r10, %rcx      // fix 3rd parameter
    push %rsp           // 8th parameter to the function
    push %rax           // 7th paramter to the function
    call c_syscall_entry
    add $16, %rsp        // caller pops parameters
    POP_CALLEE_SAVED    // restore callee saved registers
    pop %r11            // restore user rflags
    pop %rcx            // restore user rip
    pop %rsp            // restore user rsp

    swapgs
    sysretq
