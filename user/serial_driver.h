#pragma once

#include <sys/syscall.h>
#include "lib.h"

enum
{
  serial_null_cap,
  serial_tcb_cap,
  serial_cnode_cap,
  serial_serial_port_cap,
  serial_endpoint_cap,
  serial_read_notification_cap,
  serial_notification_cap,
  serial_irq_cap,
  serial_irq_notification_cap,
};

enum
{
  serial_driver_write = 1,
  serial_driver_read = 2,
};

static inline message_info_t
read_serial (cptr_t serial_endpoint, cptr_t serial_notification)
{
  while (true) {
    message_info_t info = new_message_info (serial_driver_read, 0, 0, 0);
    info = call (serial_endpoint, info, nullptr);
    if (get_message_label (info) != would_block)
      return info;

    wait (serial_notification, nullptr);
  }
}
