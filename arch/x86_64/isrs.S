#include "asm.h"

.macro isrnoerr name num
.global \name
\name:
	push $0
	push \num
	jmp interrupt_entry
.endm

.macro isrerr name num
.global \name
\name:
	push \num
	jmp interrupt_entry
.endm

.macro MAYBE_SWAPGS
    cmpq $0x23, 24(%rsp)
    jne 1f
    swapgs
1:
.endm

isrnoerr isr0   $0
isrnoerr isr1   $1
isrnoerr isr2   $2
isrnoerr isr3   $3
isrnoerr isr4   $4
isrnoerr isr5   $5
isrnoerr isr6   $6
isrnoerr isr7   $7
isrerr   isr8   $8
isrnoerr isr9   $9
isrerr   isr10 $10
isrerr   isr11 $11
isrerr   isr12 $12
isrerr   isr13 $13
isrerr   isr14 $14
isrnoerr isr15 $15
isrnoerr isr16 $16
isrerr   isr17 $17
isrnoerr isr18 $18
isrnoerr isr19 $19
isrnoerr isr20 $20
isrnoerr isr21 $21
isrnoerr isr22 $22
isrnoerr isr23 $23
isrnoerr isr24 $24
isrnoerr isr25 $25
isrnoerr isr26 $26
isrnoerr isr27 $27
isrnoerr isr28 $28
isrnoerr isr29 $29
isrerr   isr30 $30
isrnoerr isr31 $31
isrnoerr irq0  $32
isrnoerr irq1  $33
isrnoerr irq2  $34
isrnoerr irq3  $35
isrnoerr irq4  $36
isrnoerr irq5  $37
isrnoerr irq6  $38
isrnoerr irq7  $39
isrnoerr irq8  $40
isrnoerr irq9  $41
isrnoerr irq10 $42
isrnoerr irq11 $43
isrnoerr irq12 $44
isrnoerr irq13 $45
isrnoerr irq14 $46
isrnoerr irq15 $47

.global interrupt_entry
interrupt_entry:
    MAYBE_SWAPGS
    PUSH_ALL
    mov %rsp, %rdi
    call c_interrupt_entry
.return_from_interrupt:
    POP_ALL
    MAYBE_SWAPGS
    add $16, %rsp
    iretq

.global jump_to_userland_frame
jump_to_userland_frame:
    mov %rdi, %rsp
    jmp .return_from_interrupt
