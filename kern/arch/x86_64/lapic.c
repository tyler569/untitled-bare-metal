#include "kern/mem.h"
#include "x86_64.h"

#define DESTINATION_SELF 1
#define DESTINATION_ALL_INCLUDING_SELF 2
#define DESTINATION_ALL_EXCLUDING_SELF 3

constexpr uintptr_t IA32_APIC_BASE = 0x1B;

constexpr uint32_t EOI = 0xB0;
constexpr uint32_t SVR = 0xF0;
constexpr uint32_t ESR = 0x280;
constexpr uint32_t TIMER = 0x320;
constexpr uint32_t LINT0 = 0x350;
constexpr uint32_t LINT1 = 0x360;
constexpr uint32_t TIMER_ICR = 0x380;
constexpr uint32_t TIMER_DCR = 0x3E0;

constexpr uintptr_t BASE = 0xFEE00000;

static void
write_register (uint32_t reg, uint32_t value)
{
  volatile uint32_t *preg = (volatile uint32_t *)direct_map_of (BASE + reg);

  *preg = value;
}

// static uint32_t
// read_register (uint32_t reg)
// {
//   volatile uint32_t *preg = (volatile uint32_t *)direct_map_of (BASE + reg);
//
//   return *preg;
// }

static void
init_lapic_timer (uint32_t divide, uint32_t initial_count)
{
  write_register (TIMER, 0x20000 | 0x20);
  write_register (TIMER_DCR, divide);
  write_register (TIMER_ICR, initial_count);
}

void
init_lapic ()
{
  uint64_t lapic_base_mdr = read_msr (IA32_APIC_BASE);
  write_msr (IA32_APIC_BASE, lapic_base_mdr | 0x800);

  add_vm_mapping (get_vm_root (), direct_map_of (BASE), BASE,
                  PTE_PRESENT | PTE_WRITE);

  write_register (ESR, 0);
  write_register (SVR, 0x100 | 0xFF);

  write_register (LINT0, 0x8700);
  write_register (LINT1, 0x8400);

  init_lapic_timer (0x3, 100'000'000);
}

void
send_eoi (uint8_t irq)
{
  (void)irq;

  write_register (EOI, 0);
}
