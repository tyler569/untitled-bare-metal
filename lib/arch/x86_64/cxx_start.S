.text
.global _start
.type _start, @function
_start:
    mov %r15, __ipc_buffer(%rip)
    and $-16, %rsp
    call tern_main
    mov %eax, %edi
    call exit
    ud2
.size _start, .-_start
.section .note.GNU-stack,"",@progbits

.bss
.balign 8
.global __ipc_buffer
.type __ipc_buffer, @object
__ipc_buffer:
    .zero 8
.size __ipc_buffer, 8
