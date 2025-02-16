#include "stdio.h"
#include "string.h"
#include "sys/ipc.h"

#include "lib.h"
#include "serial_driver.h"

struct ipc_buffer *__ipc_buffer;

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

uint8_t buffer[1024];
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
  uint8_t buffer[256];
  for (size_t i = 0; i < get_message_length (info); i++)
    buffer[i] = get_mr (i);

  for (size_t i = 0; i < get_message_length (info); i++)
    port_write (UART_DATA, buffer[i]);
}

void
read_from_buffer ()
{
  size_t to_send = buffer_size > 100 ? 100 : buffer_size;

  message_info_t resp = new_message_info (0, 0, 0, to_send);

  for (size_t i = 0; i < to_send; i++)
    set_mr (i, buffer[i]);

  memset (buffer, 0, sizeof (buffer));
  buffer_size = 0;

  nbsend (serial_callback_endpoint_cap, resp);
}

void
handle_irq ()
{
  while (is_data_available () && buffer_size < sizeof (buffer) - 1)
    {
      const uint8_t b = port_read (UART_DATA);
      buffer[buffer_size++] = b;
    }

  irq_handler_ack (serial_irq_cap);

  if (have_receiver)
    read_from_buffer ();
}

[[noreturn]] int
main ()
{
  printf ("Hello from serial driver\n");

  set_receive_path (serial_cnode_cap, serial_staging_recv_cap, 64);
  printf ("Receive path set\n");

  initialize_uart ();
  printf ("UART initialized\n");

  irq_handler_set_notification (serial_irq_cap,
                                serial_badged_notification_cap);
  tcb_bind_notification (serial_tcb_cap, serial_badged_notification_cap);

  while (true)
    {
      message_info_t info;
      word_t badge = 0;

      info = recv (serial_endpoint_cap, &badge);

      if (badge == 0xFFFF)
        handle_irq ();
      else if (get_message_label (info) == serial_driver_write)
        write_uart (info);
      else if (get_message_label (info) == serial_driver_register)
        {
          if (get_message_extra_caps (info) != 1)
            {
              printf ("Serial: Invalid number of extra caps\n");
              continue;
            }

          cnode_copy (serial_cnode_cap, serial_callback_endpoint_cap, 64,
                      serial_cnode_cap, serial_staging_recv_cap, 64,
                      cap_rights_all);
          have_receiver = true;
        }
    }
}

__attribute__ ((naked, used)) void
_start ()
{
  asm ("mov %%r15, %0" : "=m"(__ipc_buffer));
  asm ("jmp main");
}
