#include "stdio.h"
#include "sys/ipc.h"

#include "lib.h"

struct ipc_buffer *__ipc_buffer;
extern inline long write (FILE *, const void *str, unsigned long len);

constexpr uint16_t SERIAL_PORT = 0x3f8;

constexpr uint16_t UART_DATA = 0x0;
constexpr uint16_t UART_INT_ENABLE = 0x1;

constexpr uint16_t UART_BAUD_LOW = 0x0;
constexpr uint16_t UART_BAUD_HIGH = 0x1;
constexpr uint16_t UART_FIFO_CONTROL = 0x2;
constexpr uint16_t UART_LINE_CONTROL = 0x3;
constexpr uint16_t UART_MODEM_CONTROL = 0x4;
constexpr uint16_t UART_LINE_STATUS = 0x5;
// constexpr uint16_t UART_MODEM_STATUS = 0x6;

cptr_t serial_port_cap;
cptr_t endpoint_cap;
cptr_t irq_cap;

char buffer[1024];

void
port_write (uint16_t port, uint8_t value)
{
  x86_64_io_port_out8 (serial_port_cap, SERIAL_PORT + port, value);
}

uint8_t
port_read (uint16_t port)
{
  x86_64_io_port_in8 (serial_port_cap, SERIAL_PORT + port);
  return get_mr (0);
}

void
initialize_uart ()
{
  port_write (UART_BAUD_HIGH, 0x00);
  port_write (UART_LINE_CONTROL, 0x80);
  port_write (UART_BAUD_LOW, 0x03);
  port_write (UART_BAUD_HIGH, 0x00);
  port_write (UART_LINE_CONTROL, 0x03);
  port_write (UART_FIFO_CONTROL, 0xc7);
  port_write (UART_MODEM_CONTROL, 0x0b);
  port_write (UART_INT_ENABLE, 0x09);
}

bool is_data_available ()
{
  return port_read (UART_LINE_STATUS) & 0x1;
}

void
read_uart ()
{
  while (is_data_available ())
    {
      const uint8_t b = port_read (UART_DATA);
      port_write (UART_DATA, b);
    }
  irq_handler_ack (irq_cap);
}

[[noreturn]] int
main (void *, cptr_t cap, cptr_t ep, cptr_t irq)
{
  printf ("Hello from serial driver\n");
  serial_port_cap = cap;
  endpoint_cap = ep;
  irq_cap = irq;

  initialize_uart ();
  printf ("UART initialized\n");

  while (true)
    {
      message_info_t info;
      word_t badge = 0;
      info = recv (ep, &badge);

      if (badge == 0xFFFFFFFF)
        read_uart ();
      else
        if (get_message_label (info) == 1)
           port_write (UART_DATA, get_mr (0));
    }
}

__attribute__ ((naked, used)) void
_start ()
{
  asm ("mov %%rdi, %0" : "=m"(__ipc_buffer));
  asm ("jmp main");
}
