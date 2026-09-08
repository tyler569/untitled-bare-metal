#pragma once

#include "lib.h"
#include <sys/syscall.h>

enum
{
  SERIAL_NULL_CAP,
  SERIAL_TCB_CAP,
  SERIAL_CNODE_CAP,
  SERIAL_SERIAL_PORT_CAP,
  SERIAL_ENDPOINT_CAP,
  SERIAL_READ_NOTIFICATION_CAP,
  SERIAL_NOTIFICATION_CAP,
  SERIAL_IRQ_CAP,
  SERIAL_IRQ_NOTIFICATION_CAP,
};

enum
{
  SERIAL_DRIVER_WRITE = 1,
  SERIAL_DRIVER_READ = 2,
};

static inline message_info_t
read_serial (cptr_t serial_endpoint, cptr_t serial_notification)
{
  while (true)
    {
      message_info_t info = new_message_info (SERIAL_DRIVER_READ, 0, 0, 0);
      info = call (serial_endpoint, info, nullptr);
      if (message_label (info) != WOULD_BLOCK)
        return info;

      wait (serial_notification, nullptr);
    }
}
