#include "sys/cdefs.h"
#include "x86_64.h"

#define TYPE_INTERRUPT 0xE
#define TYPE_TRAP 0xF

struct PACKED idt_entry
{
  uint16_t handler_low;
  uint16_t segment;
  uint8_t ist : 3;
  uint8_t reserved_0 : 5;
  uint8_t type : 4;
  uint8_t s : 1;
  uint8_t dpl : 2;
  uint8_t p : 1;
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
set_idt_gate (int num, uintptr_t base, int type, int dpl)
{
  idt[num].p = 1;
  idt[num].handler_low = base & 0xFFFF;
  idt[num].handler_mid = (base >> 16) & 0xFFFF;
  idt[num].handler_high = base >> 32;
  idt[num].segment = KERNEL_CS;
  idt[num].ist = 0;
  idt[num].type = type;
  idt[num].s = 0;
  idt[num].dpl = dpl;
}

void
load_idt (idt_ptr_t *i)
{
  asm volatile ("lidt %0" : : "m"(*i));
}

#define SET_IDT_GATE(n, i) set_idt_gate (n, (uintptr_t)i, TYPE_INTERRUPT, 0)

void
init_idt ()
{
  SET_IDT_GATE (0, isr0);
  SET_IDT_GATE (1, isr1);
  SET_IDT_GATE (2, isr2);
  SET_IDT_GATE (3, isr3);
  SET_IDT_GATE (4, isr4);
  SET_IDT_GATE (5, isr5);
  SET_IDT_GATE (6, isr6);
  SET_IDT_GATE (7, isr7);
  SET_IDT_GATE (8, isr8);
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
