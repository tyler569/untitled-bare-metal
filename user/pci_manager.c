#include "stdio.h"

#include "./lib.h"
#include "./pci_util.h"

static message_info_t handle_issue (message_info_t, word_t badge);
static message_info_t handle_read (message_info_t, word_t badge);
static message_info_t handle_write (message_info_t, word_t badge);
static message_info_t handle_device_info (message_info_t, word_t badge);
static message_info_t handle_enumerate (message_info_t, word_t badge);

static uint32_t pci_read_l (uint32_t addr);
static uint16_t pci_read_w (uint32_t addr);
static uint8_t pci_read_b (uint32_t addr);
static void pci_write_l (uint32_t addr, uint32_t value);
static void pci_write_w (uint32_t addr, uint16_t value);
static void pci_write_b (uint32_t addr, uint8_t value);

int
main ()
{
  bool done = false;
  message_info_t info, resp;
  word_t badge;

  info = recv (PCI_ENDPOINT_CAP, &badge);

  while (!done)
    {
      word_t label = message_label (info);

      switch (label)
        {
        case PCI_MANAGER_ISSUE:
          resp = handle_issue (info, badge);
          break;
        case PCI_MANAGER_READ:
          resp = handle_read (info, badge);
          break;
        case PCI_MANAGER_WRITE:
          resp = handle_write (info, badge);
          break;
        case PCI_MANAGER_DEVICE_INFO:
          resp = handle_device_info (info, badge);
          break;
        case PCI_MANAGER_ENUMERATE:
          resp = handle_enumerate (info, badge);
          break;
        default:
          set_mr (0, 0);
          resp = new_message_info (INVALID_ARGUMENT, 0, 0, 1);
          break;
        }

      if (done)
        reply (resp);
      else
        info = reply_recv (PCI_ENDPOINT_CAP, resp, &badge);
    }

  exit (0);
}

/*
 * in: [addr]
 * out: cap
 */
static message_info_t
handle_issue (message_info_t info, uint64_t badge)
{
  if (badge != 0)
    return new_message_info (ILLEGAL_OPERATION, 0, 0, 0);

  if (message_length (info) < 1)
    {
      set_mr (0, 1);
      return new_message_info (TRUNCATED_MESSAGE, 0, 0, 1);
    }

  cnode_delete (PCI_CNODE_CAP, PCI_TMP_CAP,
                64); // potential previous iteration
  cnode_mint (PCI_CNODE_CAP, PCI_TMP_CAP, 64, PCI_CNODE_CAP, PCI_ENDPOINT_CAP,
              64, CAP_RIGHTS_ALL, get_mr (1));
  set_cap (0, PCI_TMP_CAP);
  return new_message_info (NO_ERROR, 0, 1, 0);
}

/*
 * in: [addr, width]
 * out: [value]
 */
static message_info_t
handle_read (message_info_t info, uint64_t badge)
{
  if (message_length (info) < 2)
    {
      set_mr (0, 2);
      return new_message_info (TRUNCATED_MESSAGE, 0, 0, 1);
    }
  switch (get_mr (1))
    {
    case 1:
      set_mr (0, pci_read_b (badge | (get_mr (0) & 0xFF)));
      return new_message_info (NO_ERROR, 0, 0, 1);
    case 2:
      set_mr (0, pci_read_w (badge | (get_mr (0) & 0xFF)));
      return new_message_info (NO_ERROR, 0, 0, 1);
    case 4:
      set_mr (0, pci_read_l (badge | (get_mr (0) & 0xFF)));
      return new_message_info (NO_ERROR, 0, 0, 1);
    default:
      set_mr (0, 1);
      return new_message_info (INVALID_ARGUMENT, 0, 0, 1);
    }
}

/*
 * in: [addr, value, width]
 * out: []
 */
static message_info_t
handle_write (message_info_t info, uint64_t badge)
{
  if (message_length (info) < 3)
    {
      set_mr (0, 3);
      return new_message_info (TRUNCATED_MESSAGE, 0, 0, 1);
    }
  switch (get_mr (2))
    {
    case 1:
      pci_write_b (badge | (get_mr (0) & 0xFF), get_mr (1));
      return new_message_info (NO_ERROR, 0, 0, 0);
    case 2:
      pci_write_w (badge | (get_mr (0) & 0xFF), get_mr (1));
      return new_message_info (NO_ERROR, 0, 0, 0);
    case 4:
      pci_write_l (badge | (get_mr (0) & 0xFF), get_mr (1));
      return new_message_info (NO_ERROR, 0, 0, 0);
    default:
      set_mr (0, 1);
      return new_message_info (INVALID_ARGUMENT, 0, 0, 1);
    }
}

static uint32_t
pci_read_l (uint32_t addr)
{
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_in32 (PCI_PORT_CAP, PCI_CONFIG_DATA);
  return get_mr (0);
}

static uint16_t
pci_read_w (uint32_t addr)
{
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_in16 (PCI_PORT_CAP, PCI_CONFIG_DATA);
  return get_mr (0);
}

static uint8_t
pci_read_b (uint32_t addr)
{
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_in8 (PCI_PORT_CAP, PCI_CONFIG_DATA);
  return get_mr (0);
}

static void
pci_write_l (uint32_t addr, uint32_t value)
{
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_DATA, value);
}

static void
pci_write_w (uint32_t addr, uint16_t value)
{
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_out16 (PCI_PORT_CAP, PCI_CONFIG_DATA, value);
}

static void
pci_write_b (uint32_t addr, uint8_t value)
{
  x86_64_io_port_out32 (PCI_PORT_CAP, PCI_CONFIG_ADDRESS, addr);
  x86_64_io_port_out8 (PCI_PORT_CAP, PCI_CONFIG_DATA, value);
}

/*
 * in: [pci_address]
 * out: [config_space] (8 qwords = 64 bytes)
 */
static message_info_t
handle_device_info (message_info_t, word_t badge)
{
  uint32_t pci_address = get_mr (0);
  uint64_t pci_config_space[8];

  if (badge != 0 && badge != pci_address)
    return new_message_info (ILLEGAL_OPERATION, 0, 0, 0);

  // Read 64 bytes (0x00-0x3F) as 8 qwords
  for (uint32_t i = 0; i < 8; i++)
    {
      uint32_t low = pci_read_l (pci_address + i * 8);
      uint32_t high = pci_read_l (pci_address + i * 8 + 4);
      pci_config_space[i] = (uint64_t)high << 32 | low;
    }

  for (size_t i = 0; i < 8; i++)
    set_mr (i, pci_config_space[i]);

  return new_message_info (NO_ERROR, 0, 0, 8);
}

/*
 * in: []
 * out: [(vendor.device.addr)...] packed
 */
static message_info_t
handle_enumerate (message_info_t, word_t badge)
{
  if (badge != 0)
    return new_message_info (ILLEGAL_OPERATION, 0, 0, 0);

  uint32_t mr = 0;
  uint64_t mrs[64];

  for (uint32_t bus = 0; bus < 256; bus++)
    {
      if (pci_read_l (pci_addr (bus, 0, 0, 0)) == 0xFFFFFFFF)
        continue;

      for (uint32_t dev = 0; dev < 32; dev++)
        for (uint32_t func = 0; func < 8; func++)
          {
            uint32_t addr = pci_addr (bus, dev, func, 0);
            uint32_t info = pci_read_l (addr);
            if (info != 0xFFFFFFFF)
              mrs[mr++] = (uint64_t)addr << 32 | info;
            if (mr >= MESSAGE_MAX_LENGTH)
              goto done;
          }
    }
done:

  for (size_t i = 0; i < mr; i++)
    set_mr (i, mrs[i]);

  return new_message_info (NO_ERROR, 0, 0, mr);
}
