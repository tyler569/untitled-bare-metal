#include "stdio.h"
#include "string.h"
#include "sys/ipc.h"

#include "lib.h"
#include "serial_driver.h"

constexpr uint16_t SERIAL_PORT = 0x3F8;

constexpr uint16_t UART_DATA = 0;
constexpr uint16_t UART_INT_ENABLE = 1;

constexpr uint16_t UART_BAUD_LOW = 0;
constexpr uint16_t UART_BAUD_HIGH = 1;
constexpr uint16_t UART_FIFO_CONTROL = 2;
constexpr uint16_t UART_LINE_CONTROL = 3;
constexpr uint16_t UART_MODEM_CONTROL = 4;
constexpr uint16_t UART_LINE_STATUS = 5;
// constexpr uint16_t UART_MODEM_STATUS = 6;

constexpr size_t SERIAL_BUFFER_SIZE = 256;

char buffer[SERIAL_BUFFER_SIZE];
size_t buffer_size = 0;

bool have_receiver = false;

void
port_write (uint16_t port, uint8_t value)
{
  x86_64_io_port_out8 (SERIAL_SERIAL_PORT_CAP, SERIAL_PORT + port, value);
}

uint8_t
port_read (uint16_t port)
{
  x86_64_io_port_in8 (SERIAL_SERIAL_PORT_CAP, SERIAL_PORT + port);
  return get_mr (0);
}

void
initialize_uart ()
{
  port_write (UART_BAUD_HIGH, 0);
  port_write (UART_LINE_CONTROL, 0x80);
  port_write (UART_BAUD_LOW, 3);
  port_write (UART_BAUD_HIGH, 0);
  port_write (UART_LINE_CONTROL, 3);
  port_write (UART_FIFO_CONTROL, 0xC7);
  port_write (UART_MODEM_CONTROL, 0xB);
  port_write (UART_INT_ENABLE, 9);
}

bool
is_data_available ()
{
  return port_read (UART_LINE_STATUS) & 1;
}

void
write_uart (message_info_t info)
{
  // These can't be interleaved because it will wipe out the IPC buffer

  uint8_t buffer[128];
  for (size_t i = 0; i < message_length (info); i++)
    buffer[i] = get_mr (i);

  for (size_t i = 0; i < message_length (info); i++)
    port_write (UART_DATA, buffer[i]);
}

void
read_uart (message_info_t)
{
  if (buffer_size == 0)
    {
      reply (new_message_info (WOULD_BLOCK, 0, 0, 0));
      return;
    }

  size_t i;
  for (i = 0; i < buffer_size && i < MESSAGE_MAX_LENGTH; i++)
    set_mr (i, buffer[i]);
  buffer_size = 0;

  reply (new_message_info (0, 0, 0, i));
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

      signal (SERIAL_READ_NOTIFICATION_CAP);
    }

  irq_handler_ack (SERIAL_IRQ_CAP);
}

[[noreturn]] int
driver_thread_main ()
{
  printf ("Hello from serial driver\n");

  initialize_uart ();
  printf ("UART initialized\n");

  irq_handler_set_notification (SERIAL_IRQ_CAP, SERIAL_IRQ_NOTIFICATION_CAP);
  tcb_bind_notification (SERIAL_TCB_CAP, SERIAL_IRQ_NOTIFICATION_CAP);

  while (true)
    {
      message_info_t info;
      word_t badge = 0;

      info = recv (SERIAL_ENDPOINT_CAP, &badge);

      if (badge == 0xFFFF)
        handle_irq ();
      else if (message_label (info) == SERIAL_DRIVER_WRITE)
        write_uart (info);
      else if (message_label (info) == SERIAL_DRIVER_READ)
        read_uart (info);
    }
}

int
main ()
{
  driver_thread_main ();

  exit (1);
}
