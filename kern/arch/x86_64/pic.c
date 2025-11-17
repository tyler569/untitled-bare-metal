#include "x86_64.h"

#define PRIMARY_COMMAND 0x20
#define PRIMARY_DATA 0x21
#define SECONDARY_COMMAND 0xA0
#define SECONDARY_DATA 0xA1

void
init_pic ()
{
  write_port_b (PRIMARY_COMMAND, 0x11); // reset
  write_port_b (PRIMARY_DATA, 0x20);    // interrupt 0x20
  write_port_b (PRIMARY_DATA, 0x04);    // secondary at IRQ2
  write_port_b (PRIMARY_DATA, 0x01);    // 8086 mode
  write_port_b (PRIMARY_DATA, 0xFF);    // mask all interrupts

  write_port_b (SECONDARY_COMMAND, 0x11); // reset
  write_port_b (SECONDARY_DATA, 0x28);    // interrupt 0x28
  write_port_b (SECONDARY_DATA, 0x02);    // cascade identity
  write_port_b (SECONDARY_DATA, 0x01);    // 8086 mode
  write_port_b (SECONDARY_DATA, 0xFF);    // mask all interrupts
}
