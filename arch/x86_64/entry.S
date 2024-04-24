.macro isrnoerr name num handler
.global \name
\name:
	push $0
	push \num
	jmp \handler
.endm

.macro isrerr name num handler
.global \name
\name:
	push \num
	jmp \handler
.endm


isrnoerr isr0   $0 interrupt_entry
isrnoerr isr1   $1 interrupt_entry
isrnoerr isr2   $2 interrupt_entry
isrnoerr isr3   $3 interrupt_entry
isrnoerr isr4   $4 interrupt_entry
isrnoerr isr5   $5 interrupt_entry
isrnoerr isr6   $6 interrupt_entry
isrnoerr isr7   $7 interrupt_entry
isrerr   isr8   $8 interrupt_entry
isrnoerr isr9   $9 interrupt_entry
isrerr   isr10 $10 interrupt_entry
isrerr   isr11 $11 interrupt_entry
isrerr   isr12 $12 interrupt_entry
isrerr   isr13 $13 interrupt_entry
isrerr   isr14 $14 interrupt_entry
isrnoerr isr15 $15 interrupt_entry
isrnoerr isr16 $16 interrupt_entry
isrerr   isr17 $17 interrupt_entry
isrnoerr isr18 $18 interrupt_entry
isrnoerr isr19 $19 interrupt_entry
isrnoerr isr20 $20 interrupt_entry
isrnoerr isr21 $21 interrupt_entry
isrnoerr isr22 $22 interrupt_entry
isrnoerr isr23 $23 interrupt_entry
isrnoerr isr24 $24 interrupt_entry
isrnoerr isr25 $25 interrupt_entry
isrnoerr isr26 $26 interrupt_entry
isrnoerr isr27 $27 interrupt_entry
isrnoerr isr28 $28 interrupt_entry
isrnoerr isr29 $29 interrupt_entry
isrerr   isr30 $30 interrupt_entry
isrnoerr isr31 $31 interrupt_entry
isrnoerr irq0  $32 interrupt_entry
isrnoerr irq1  $33 interrupt_entry
isrnoerr irq2  $34 interrupt_entry
isrnoerr irq3  $35 interrupt_entry
isrnoerr irq4  $36 interrupt_entry
isrnoerr irq5  $37 interrupt_entry
isrnoerr irq6  $38 interrupt_entry
isrnoerr irq7  $39 interrupt_entry
isrnoerr irq8  $40 interrupt_entry
isrnoerr irq9  $41 interrupt_entry
isrnoerr irq10 $42 interrupt_entry
isrnoerr irq11 $43 interrupt_entry
isrnoerr irq12 $44 interrupt_entry
isrnoerr irq13 $45 interrupt_entry
isrnoerr irq14 $46 interrupt_entry
isrnoerr irq15 $47 interrupt_entry
isrnoerr isr_syscall $128 syscall_entry

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


.global interrupt_entry
interrupt_entry:
    PUSH_ALL
    mov %rsp, %rdi
    call c_interrupt_entry
    POP_ALL
    add $16, %rsp
    iretq


.global syscall_entry
syscall_entry:
    PUSH_CALLEE_SAVED
    mov %rsp, %rdi
    call c_syscall_entry
    POP_CALLEE_SAVED
    add $16, %rsp
    iretq
