#include <stdatomic.h>
#include "stdio.h"
#include "string.h"
#include "sys/ipc.h"
#include "sys/types.h"

#include "lib.h"
#include "pci_util.h"
// #include "e1000e_driver.h"

struct ipc_buffer *__ipc_buffer;

// HEADER BEGIN

// HEADER END

struct mac_addr
{
  uint8_t addr[6];
};

constexpr size_t tx_desc_count = 80;
constexpr size_t rx_desc_count = 80;
constexpr size_t tx_buffer_size = 2048;
constexpr size_t rx_buffer_size = 2048;

struct e1000e_tx_desc
{
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
};

struct e1000e_rx_desc
{
  uint64_t addr;
  uint16_t length;
  uint16_t checksum;
  uint8_t status;
  uint8_t errors;
  uint16_t special;
};

enum e1000e_registers
{
  CTRL = 0x00000,
  STATUS = 0x00008,
  EECD = 0x00010,
  EERD = 0x00014,
  FCT = 0x00030,
  VET = 0x00038,
  ICR = 0x000C0,
  ITR = 0x000C4,
  ICS = 0x000C8,
  IMS = 0x000D0,
  IMC = 0x000D8,
  RCTL = 0x00100,
  TCTL = 0x00400,
  TIPG = 0x00410,

  FCRTL = 0x02160,
  FCRTH = 0x02168,

  RDBAL = 0x02800,
  RDBAH = 0x02804,
  RDLEN = 0x02808,
  RDH = 0x02810,
  RDT = 0x02818,
  RDTR = 0x02820,
  RADV = 0x0282C,
  RSRPD = 0x02C00,

  TDBAL = 0x03800,
  TDBAH = 0x03804,
  TDLEN = 0x03808,
  TDH = 0x03810,
  TDT = 0x03818,

  RAL = 0x05400,
  RAH = 0x05404,
}

enum e1000e_ctrl
{
  CTRL_SLU = 1 << 6,
  CTRL_SPEED_1000 = 1 << 9,
  CTRL_RST = 1 << 26,
  CTRL_PHY_RST = 1 << 31,
}

static uintptr_t mmio_base;

void write_l (uintptr_t offset, uint32_t value)
{
  *(volatile uint32_t *)(mmio_base + offset) = value;
}

uint32_t read_l (uintptr_t offset)
{
  return *(volatile uint32_t *)(mmio_base + offset);
}

void
reset ()
{
  write_l (IMS, 0xffff);
  write_l (IMC, 0xffff);

  write_l (CTRL, CTRL_RESET);
  while (read_l (CTRL) & CTRL_RESET)
    ;

  write_l (IMC, 0xffff);

  __atomic_thread_fence (memory_order_seq_cst);
}

uint16_t
read_eeprom (uint8_t addr)
{
  write_l (EERD, (1 << 8) | addr);
  while (!(read_l (EERD) & (1 << 4)))
    ;

  return read_l (EERD) >> 16;
}

struct mac_addr
read_mac ()
{
  uint16_t mac[3];
  mac[0] = read_eeprom (0);
  mac[1] = read_eeprom (1);
  mac[2] = read_eeprom (2);

  struct mac_addr addr;
  memcpy (addr.addr, mac, sizeof (mac));
  return addr;
}

void
initialize_e1000e ()
{
  reset ();
}

[[noreturn]] int
main (uint32_t pci_addr)
{
  printf ("Hello from e1000e driver\n");

  initialize_uart ();
  printf ("e1000e initialized\n");

  // irq_handler_set_notification (e1000e_irq_cap, e1000e_irq_notification_cap);
  // tcb_bind_notification (e1000e_tcb_cap, e1000e_irq_notification_cap);

  // while (true)
  //   {
  //   }
  
  struct mac_addr mac = read_mac ();
  printf ("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac.addr[0], mac.addr[1], mac.addr[2],
           mac.addr[3], mac.addr[4], mac.addr[5]);

  tcb_suspend (e1000e_tcb_cap);
}

__attribute__ ((naked, used)) void
_start ()
{
  asm ("mov %%r15, %0" : "=m"(__ipc_buffer));
  asm ("jmp main");
}
