#include "pci.h"
#include "lib.h"

uint32_t
pci_read_l (cptr_t port, uint32_t addr)
{
  x86_64_io_port_out32 (port, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_in32 (port, PCI_CONFIG_DATA);
  return get_mr (0);
}

uint16_t
pci_read_w (cptr_t port, uint32_t addr)
{
  x86_64_io_port_out32 (port, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_in16 (port, PCI_CONFIG_DATA);
  return get_mr (0);
}

uint8_t
pci_read_b (cptr_t port, uint32_t addr)
{
  x86_64_io_port_out32 (port, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_in8 (port, PCI_CONFIG_DATA);
  return get_mr (0);
}

void
pci_write_l (cptr_t port, uint32_t addr, uint32_t value)
{
  x86_64_io_port_out32 (port, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_out32 (port, PCI_CONFIG_DATA, value);
}

void
print_device_info (cptr_t port, uint32_t pci_address)
{
  uint16_t vendor_id = pci_read_w (port, pci_address + PCI_VENDOR_ID);
  uint16_t device_id = pci_read_w (port, pci_address + PCI_DEVICE_ID);
  uint8_t class_code = pci_read_w (port, pci_address + PCI_CLASS);
  uint8_t subclass = pci_read_w (port, pci_address + PCI_SUBCLASS);
  uint8_t prog_if = pci_read_w (port, pci_address + PCI_PROG_IF);
  uint8_t revision_id = pci_read_w (port, pci_address + PCI_REVISION_ID);
  uint8_t irq = pci_read_b (port, pci_address + PCI_INTERRUPT_LINE);
  uint16_t status = pci_read_w (port, pci_address + PCI_STATUS);

  printf ("PCI Device: %04x:%04x at addr %08x\n", vendor_id, device_id,
          pci_address);
  printf ("  Class: %02x, Subclass: %02x, Prog IF: %02x, Revision: %02x IRQ: "
          "%02x Status: %04x\n",
          class_code, subclass, prog_if, revision_id, irq, status);

  for (uint32_t i = 0; i < 6; i++)
    {
      uint32_t bar = pci_read_l (port, pci_address + PCI_BAR0 + i * 4);
      if (bar)
        {
          pci_write_l (port, pci_address + PCI_BAR0 + i * 4, 0xffffffff);
          uint32_t mask = pci_read_l (port, pci_address + PCI_BAR0 + i * 4);
          pci_write_l (port, pci_address + PCI_BAR0 + i * 4, bar);
          bool is_io = bar & 1;
          if (bar & 1)
            {
              bar &= 0xfffffffc;
              mask &= 0xfffffffc;
            }
          else
            {
              mask &= 0xfffffff0;
              bar &= 0xfffffff0;
            }

          uint32_t size = ~mask + 1;

          if (is_io)
            printf ("  - BAR%d: I/O %08x, Mask: %08x, Size: %x\n", i, bar,
                    mask, size);
          else
            printf ("  - BAR%d: Mem %08x, Mask: %08x, Size: %x\n", i, bar,
                    mask, size);
        }
    }
  if (status & (1 << 4))
    {
      uint8_t capabilities_pointer
          = pci_read_b (port, pci_address + PCI_CAPABILITY_LIST);
      while (capabilities_pointer)
        {
          uint8_t capability_id
              = pci_read_b (port, pci_address + capabilities_pointer);
          uint8_t next_capability
              = pci_read_b (port, pci_address + capabilities_pointer + 1);
          printf ("  - Capability: %02x at %02x\n", capability_id,
                  capabilities_pointer);
          capabilities_pointer = next_capability;
        }
    }
}

void
enumerate_pci_bus (cptr_t port)
{
  for (uint32_t bus = 0; bus < 256; bus++)
	{
	  if (pci_read_l (port, pci_addr (bus, 0, 0, 0)) == 0xffffffff)
		continue;

      for (uint32_t dev = 0; dev < 32; dev++)
        for (uint32_t func = 0; func < 8; func++)
          {
            uint32_t addr = pci_addr (bus, dev, func, 0);
            if (pci_read_l (port, addr) != 0xffffffff)
              print_device_info (port, addr);
          }
	}
}
