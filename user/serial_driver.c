#include "stdio.h"
#include "string.h"
#include "sys/ipc.h"

#include "lib.h"
#include "serial_driver.h"

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

constexpr size_t SERIAL_BUFFER_SIZE = 256;

char buffer[SERIAL_BUFFER_SIZE];
size_t buffer_size = 0;

bool have_receiver = false;

void
port_write (uint16_t port, uint8_t value)
{
  x86_64_io_port_out8 (serial_serial_port_cap, SERIAL_PORT + port, value);
}

uint8_t
port_read (uint16_t port)
{
  x86_64_io_port_in8 (serial_serial_port_cap, SERIAL_PORT + port);
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

bool
is_data_available ()
{
  return port_read (UART_LINE_STATUS) & 0x1;
}

void
write_uart (message_info_t info)
{
  // These can't be interleaved because it will wipe out the IPC buffer

  uint8_t buffer[128];
  for (size_t i = 0; i < get_message_length (info); i++)
    buffer[i] = get_mr (i);

  for (size_t i = 0; i < get_message_length (info); i++)
    port_write (UART_DATA, buffer[i]);
}

void
handle_irq ()
{
  while (is_data_available ())
    {
      while (is_data_available () && buffer_size < SERIAL_BUFFER_SIZE)
        {
          const uint8_t b = port_read (UART_DATA);
          buffer[buffer_size++] = b;
        }

      serial_driver_ring_write (shared_ring, buffer, buffer_size);
      signal (serial_broker_notification_cap);
      buffer_size = 0;
    }

  irq_handler_ack (serial_irq_cap);
}

[[noreturn]] int
main ()
{
  printf ("Hello from serial driver\n");

  initialize_uart ();
  printf ("UART initialized\n");

  irq_handler_set_notification (serial_irq_cap, serial_irq_notification_cap);
  tcb_bind_notification (serial_tcb_cap, serial_irq_notification_cap);

  while (true)
    {
      message_info_t info;
      word_t badge = 0;

      info = recv (serial_endpoint_cap, &badge);

      if (badge == 0xFFFF)
        handle_irq ();
      else if (get_message_label (info) == serial_driver_write)
        write_uart (info);
    }
}
