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
// constexpr uint16_t UART_LINE_STATUS = 0x5;
// constexpr uint16_t UART_MODEM_STATUS = 0x6;

cptr_t serial_port_cap;
cptr_t endpoint_cap;

char buffer[1024];

void
port_write (uint16_t port, uint8_t value)
{
  x86_64_io_port_out8 (serial_port_cap, SERIAL_PORT + port, value);
}

// uint8_t
// port_read (uint16_t port)
// {
//   return x86_64_io_port_in8 (serial_port_cap, SERIAL_PORT + port);
// }

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

void
write_uart (const char c)
{
  port_write (UART_DATA, c);
}

int
main (void *, cptr_t cap, cptr_t ep)
{
  printf ("Hello from serial driver\n");
  serial_port_cap = cap;
  endpoint_cap = ep;

  initialize_uart ();
  printf ("UART initialized\n");

  while (true)
    {
      message_info_t info;
      word_t badge;
      info = recv (ep, &badge);

      switch (get_message_label (info))
        {
        case 1:
          write_uart ((char)get_mr (0));
          break;
        default:
          break;
        }
    }
}

__attribute__ ((naked)) void
_start ()
{
  asm ("mov %%rdi, %0" : "=m"(__ipc_buffer));
  asm ("jmp main");
}
