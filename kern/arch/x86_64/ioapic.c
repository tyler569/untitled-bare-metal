#include "kern/mem.h"
#include "x86_64.h"

constexpr uintptr_t IOAPIC_BASE = 0xFEC00000;

static void
write_register (uint32_t reg, uint32_t data)
{
  volatile uint32_t *ioapic_reg
      = (volatile uint32_t *)(direct_map_of (IOAPIC_BASE));
  volatile uint32_t *ioapic_data
      = (volatile uint32_t *)(direct_map_of (IOAPIC_BASE + 0x10));

  *ioapic_reg = reg;
  *ioapic_data = data;
}

// static uint32_t
// read_register (uint32_t reg)
// {
//   assert (reg < 0x40);
//   volatile uint32_t *ioapic_reg
//       = (volatile uint32_t *)(direct_map_of (IOAPIC_BASE));
//   volatile uint32_t *ioapic_data
//       = (volatile uint32_t *)(direct_map_of (IOAPIC_BASE + 0x10));
//
//   *ioapic_reg = reg;
//   return *ioapic_data;
// }

union ioapic_relocation_entry
{
  struct
  {
    uint64_t vector : 8;
    uint64_t delivery_mode : 3;
    uint64_t dest_mode : 1;
    uint64_t delivery_status : 1;
    uint64_t pin_polarity : 1;
    uint64_t remote_irr : 1;
    uint64_t trigger_mode : 1;
    uint64_t mask : 1;
    uint64_t reserved : 39;
    uint64_t dest : 8;
  };
  struct
  {
    uint32_t low;
    uint32_t high;
  };
};

static void
write_relocation_entry (uint32_t irq, union ioapic_relocation_entry entry)
{
  uint32_t reg = 0x10 + irq * 2;

  write_register (reg, entry.low);
  write_register (reg + 1, entry.high);
}

void
init_ioapic ()
{
  add_vm_mapping (get_vm_root (), direct_map_of (IOAPIC_BASE), IOAPIC_BASE,
                  PTE_PRESENT | PTE_WRITE);

  for (int i = 1; i < 24; i++)
    {
      union ioapic_relocation_entry entry = {
        .vector = i + 0x20,
        .dest = 0,
      };

      write_relocation_entry (i, entry);
    }

  // TODO handle acpi mappings

  union ioapic_relocation_entry entry = {
    .vector = 0x20,
    .mask = 1,
  };

  write_relocation_entry (2, entry);
}
