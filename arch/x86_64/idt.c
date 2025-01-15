#include "kern/mem.h"
#include "sys/cdefs.h"
#include "x86_64.h"

#define TYPE_INT 0x8E
#define TYPE_USER_INT 0x60 | 0x8E

#define INT_STACK_SIZE (PAGE_SIZE * 2)

struct PACKED idt_entry
{
  uint16_t handler_low;
  uint16_t segment;
  uint8_t ist;
  uint8_t type;
  uint16_t handler_mid;
  uint32_t handler_high;
  uint32_t reserved_1;
};

struct PACKED idt_ptr
{
  uint16_t limit;
  uintptr_t base;
};

typedef struct idt_entry idt_entry_t;
typedef struct idt_ptr idt_ptr_t;

static idt_entry_t idt[48];
static idt_ptr_t idt_ptr = { sizeof (idt) - 1, (uintptr_t)idt };

void
set_idt_gate (int num, uintptr_t base, int type, int ist)
{
  idt[num].handler_low = base & 0xFFFF;
  idt[num].handler_mid = (base >> 16) & 0xFFFF;
  idt[num].handler_high = base >> 32;
  idt[num].segment = KERNEL_CS;
  idt[num].ist = ist;
  idt[num].type = type;
}

void
load_idt (idt_ptr_t *i)
{
  asm volatile ("lidt %0" : : "m"(*i));
}

#define SET_IDT_GATE(n, i) set_idt_gate (n, (uintptr_t)i, TYPE_INT, 0)
#define SET_IDT_GATE_ex(n, i, t, ist) set_idt_gate (n, (uintptr_t)i, t, ist)

void
init_idt ()
{
  SET_IDT_GATE (0, isr0);
  SET_IDT_GATE (1, isr1);
  SET_IDT_GATE_ex (2, isr2, TYPE_INT, IST_NMI);
  SET_IDT_GATE_ex (3, isr3, TYPE_USER_INT, 0);
  SET_IDT_GATE (4, isr4);
  SET_IDT_GATE (5, isr5);
  SET_IDT_GATE (6, isr6);
  SET_IDT_GATE (7, isr7);
  SET_IDT_GATE_ex (8, isr8, TYPE_INT, IST_DF);
  SET_IDT_GATE (9, isr9);
  SET_IDT_GATE (10, isr10);
  SET_IDT_GATE (11, isr11);
  SET_IDT_GATE (12, isr12);
  SET_IDT_GATE (13, isr13);
  SET_IDT_GATE (14, isr14);
  SET_IDT_GATE (15, isr15);
  SET_IDT_GATE (16, isr16);
  SET_IDT_GATE (17, isr17);
  SET_IDT_GATE (18, isr18);
  SET_IDT_GATE (19, isr19);
  SET_IDT_GATE (20, isr20);
  SET_IDT_GATE (21, isr21);
  SET_IDT_GATE (22, isr22);
  SET_IDT_GATE (23, isr23);
  SET_IDT_GATE (24, isr24);
  SET_IDT_GATE (25, isr25);
  SET_IDT_GATE (26, isr26);
  SET_IDT_GATE (27, isr27);
  SET_IDT_GATE (28, isr28);
  SET_IDT_GATE (29, isr29);
  SET_IDT_GATE (30, isr30);
  SET_IDT_GATE (31, isr31);
  SET_IDT_GATE (32, irq0);
  SET_IDT_GATE (33, irq1);
  SET_IDT_GATE (34, irq2);
  SET_IDT_GATE (35, irq3);
  SET_IDT_GATE (36, irq4);
  SET_IDT_GATE (37, irq5);
  SET_IDT_GATE (38, irq6);
  SET_IDT_GATE (39, irq7);
  SET_IDT_GATE (40, irq8);
  SET_IDT_GATE (41, irq9);
  SET_IDT_GATE (42, irq10);
  SET_IDT_GATE (43, irq11);
  SET_IDT_GATE (44, irq12);
  SET_IDT_GATE (45, irq13);
  SET_IDT_GATE (46, irq14);
  SET_IDT_GATE (47, irq15);

  load_idt (&idt_ptr);
}

void
init_ap_idt ()
{
  load_idt (&idt_ptr);
}

char __attribute__ ((aligned (16))) int_stacks[INT_STACK_SIZE * 3];

void
init_int_stacks ()
{
  void *int_stack = int_stacks;
  void *nmi_stack = int_stacks + INT_STACK_SIZE;
  void *df_stack = int_stacks + 2 * INT_STACK_SIZE;

  this_cpu->kernel_stack_top = (uintptr_t)int_stack + INT_STACK_SIZE;
  this_cpu->arch.tss.rsp[0] = this_cpu->kernel_stack_top;
  this_cpu->arch.tss.ist[0] = (uintptr_t)nmi_stack + INT_STACK_SIZE;
  this_cpu->arch.tss.ist[0] = (uintptr_t)df_stack + INT_STACK_SIZE;
}
