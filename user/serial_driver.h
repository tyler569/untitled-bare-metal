#pragma once

enum
{
  serial_null_cap,
  serial_tcb_cap,
  serial_cnode_cap,
  serial_serial_port_cap,
  serial_endpoint_cap,
  serial_notification_cap,
  serial_badged_notification_cap,
  serial_irq_cap,
  serial_staging_recv_cap,
  serial_callback_endpoint_cap,
};

enum
{
  serial_driver_write = 1,
  serial_driver_register = 2,
};
