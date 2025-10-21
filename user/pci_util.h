#include "lib.h"
#include "pci.h"
#include "sys/types.h"

uint32_t pci_read_l (cptr_t port, uint32_t addr);
uint16_t pci_read_w (cptr_t port, uint32_t addr);
uint8_t pci_read_b (cptr_t port, uint32_t addr);
void pci_write_l (cptr_t port, uint32_t addr, uint32_t value);
void pci_write_w (cptr_t port, uint32_t addr, uint16_t value);
void pci_write_b (cptr_t port, uint32_t addr, uint8_t value);

void print_device_info (cptr_t port, uint32_t pci_address);
void enumerate_pci_bus (cptr_t port);
