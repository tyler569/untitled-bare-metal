#include "asm.h"

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


isrnoerr isr0   $0 exception_entry
isrnoerr isr1   $1 exception_entry
isrnoerr isr2   $2 exception_entry
isrnoerr isr3   $3 exception_entry
isrnoerr isr4   $4 exception_entry
isrnoerr isr5   $5 exception_entry
isrnoerr isr6   $6 exception_entry
isrnoerr isr7   $7 exception_entry
isrerr   isr8   $8 exception_entry
isrnoerr isr9   $9 exception_entry
isrerr   isr10 $10 exception_entry
isrerr   isr11 $11 exception_entry
isrerr   isr12 $12 exception_entry
isrerr   isr13 $13 exception_entry
isrerr   isr14 $14 exception_entry
isrnoerr isr15 $15 exception_entry
isrnoerr isr16 $16 exception_entry
isrerr   isr17 $17 exception_entry
isrnoerr isr18 $18 exception_entry
isrnoerr isr19 $19 exception_entry
isrnoerr isr20 $20 exception_entry
isrnoerr isr21 $21 exception_entry
isrnoerr isr22 $22 exception_entry
isrnoerr isr23 $23 exception_entry
isrnoerr isr24 $24 exception_entry
isrnoerr isr25 $25 exception_entry
isrnoerr isr26 $26 exception_entry
isrnoerr isr27 $27 exception_entry
isrnoerr isr28 $28 exception_entry
isrnoerr isr29 $29 exception_entry
isrerr   isr30 $30 exception_entry
isrnoerr isr31 $31 exception_entry
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

.global exception_entry
exception_entry:
    PUSH_ALL
    mov %rsp, %rdi
    call c_exception_entry
    POP_ALL
    add $16, %rsp
    iretq


.global interrupt_entry
interrupt_entry:
    PUSH_ALL
    mov %rsp, %rdi
    call c_interrupt_entry
    POP_ALL
    add $16, %rsp
    iretq
