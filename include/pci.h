#pragma once

#include "stdint.h"

constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

constexpr uint16_t PCI_VENDOR_ID = 0;
constexpr uint16_t PCI_DEVICE_ID = 2;
constexpr uint16_t PCI_COMMAND = 4;
constexpr uint16_t PCI_STATUS = 6;
constexpr uint16_t PCI_REVISION_ID = 8;
constexpr uint16_t PCI_PROG_IF = 9;
constexpr uint16_t PCI_SUBCLASS = 0xA;
constexpr uint16_t PCI_CLASS = 0xB;
constexpr uint16_t PCI_CACHE_LINE_SIZE = 0xC;
constexpr uint16_t PCI_LATENCY_TIMER = 0xD;
constexpr uint16_t PCI_HEADER_TYPE = 0xE;
constexpr uint16_t PCI_BIST = 0xF;
constexpr uint16_t PCI_BAR0 = 0x10;
constexpr uint16_t PCI_BAR1 = 0x14;
constexpr uint16_t PCI_BAR2 = 0x18;
constexpr uint16_t PCI_BAR3 = 0x1C;
constexpr uint16_t PCI_BAR4 = 0x20;
constexpr uint16_t PCI_BAR5 = 0x24;
constexpr uint16_t PCI_CAPABILITY_LIST = 0x34;
constexpr uint16_t PCI_INTERRUPT_LINE = 0x3C;
constexpr uint16_t PCI_INTERRUPT_PIN = 0x3D;

static inline uint32_t
pci_addr (uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
  return (1U << 31) | (bus << 16) | (device << 11) | (function << 8)
         | (offset & 0xFC);
}

static inline uint32_t
pci_bus (uint32_t address)
{
  return (address >> 16) & 0xFF;
}

static inline uint32_t
pci_device (uint32_t address)
{
  return (address >> 11) & 0x1F;
}

static inline uint32_t
pci_function (uint32_t address)
{
  return (address >> 8) & 7;
}

static inline uint32_t
pci_offset (uint32_t address)
{
  return address & 0xFC;
}
