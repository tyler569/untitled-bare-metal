#include "stdio.h"
#include "string.h"
#include "sys/ipc.h"

#include "lib.h"

constexpr size_t TX_DESC_COUNT = 80;
constexpr size_t RX_DESC_COUNT = 80;
constexpr size_t TX_BUFFER_SIZE = 2048;
constexpr size_t RX_BUFFER_SIZE = 2048;

struct tx_desc
{
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
};

struct rx_desc
{
  uint64_t addr;
  uint16_t length;
  uint16_t csum;
  uint8_t status;
  uint8_t errors;
  uint16_t special;
};

static_assert (sizeof (struct tx_desc) == 16);
static_assert (sizeof (struct rx_desc) == 16);

enum reg
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
};

enum ctrl
{
  CTRL_SLU = 1 << 6,
  CTRL_SPEED_1000 = 1 << 9,
  CTRL_RST = 1 << 26,
  CTRL_PHY_RST = 1 << 31,
};

enum interrupt
{
  INT_TXDW = 1 << 0, // Transmit descriptor written back
  INT_TXQE = 1 << 1, // Transmit queue empty
  INT_LSC = 1 << 2, // Link status change
  INT_RXSEQ = 1 << 3, // Receive sequence error
  INT_RXDMTO = 1 << 4, // Receive descriptor minimum threshold
  INT_RXO = 1 << 6, // Receive overrun
  INT_RXTO = 1 << 7, // Receive timer interrupt
  INT_MDAC = 1 << 9, // MDIO access complete
  INT_RXCFG = 1 << 10, // Receive configuration change
  INT_PHY = 1 << 12, // PHY interrupt
  INT_GPI = 1 << 13, // General purpose interrupt
  INT_TXD_LOW = 1 << 15, // Transmit descriptor low
  INT_SRPD = 1 << 16, // Small receive packet detected

  INT_ALL = 0x0000FFFF,
};

enum cmd
{
  CMD_EOP = 1 << 0, // End of packet
  CMD_IFCS = 1 << 1, // Insert FCS
  CMD_IC = 1 << 2, // Insert checksum
  CMD_RS = 1 << 3, // Report status
  CMD_RPS = 1 << 4, // Report packet sent
  CMD_DEXT = 1 << 5, // Descriptor extension
  CMD_VLE = 1 << 6, // VLAN packet enable
  CMD_IDE = 1 << 7, // Interrupt delay enable
};

void *mmio_base;

static inline uint32_t
read_reg (enum reg offset)
{
  return *(volatile uint32_t *)(mmio_base + reg);
}

static inline void
write_reg (enum reg offset, uint32_t value)
{
  *(volatile uint32_t *)(mmio_base + reg) = value;
}

void
reset ()
{
  write_reg (IMS, INT_ALL);
  write_reg (IMC, INT_ALL);

  write_reg (CTRL, CTRL_RESET);
  while (read_reg (CRTL) & CTRL_RESET)
    asm volatile ("pause");

  for (int i = 0; i < 100'000; i++)
    asm volatile ("pause");

  write_reg (IMC, INT_ALL);

  __atomic_thread_fence (memory_order_seq_cst);
}

void
initialize ()
{
  // pci_read/write
}

void
write_packet ()
{
}

void
read_packet ()
{
}

void
handle_irq ()
{
}

int
main ()
{
}
