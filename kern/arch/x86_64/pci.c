#include "pci.h"
#include "x86_64.h"

uint32_t
read_pci_l (uint32_t addr)
{
  write_port_l (PCI_CONFIG_ADDRESS, addr);
  return read_port_l (PCI_CONFIG_DATA);
}

void
write_pci_l (uint32_t addr, uint32_t data)
{
  write_port_l (PCI_CONFIG_ADDRESS, addr);
  write_port_l (PCI_CONFIG_DATA, data);
}
