#include "../x86_64.h"
#include "kern/cap.h"
#include "kern/syscall.h"

error_t
x86_64_io_port_control_issue (cte_t *, uint16_t first_port, uint16_t last_port,
                              cte_t *root, word_t index, word_t depth)
{
  cte_t *dest_slot;
  error_t err = lookup_cap_slot (root, index, depth, &dest_slot);
  if (err)
    return err;

  dest_slot->cap = cap_x86_64_io_port_new (first_port, last_port);

  return no_error;
}

static error_t 
x86_64_io_port_range_check (cte_t *slot, word_t port, word_t size)
{
  uint16_t first_port = cap_x86_64_io_port_first_port (slot->cap);
  uint16_t last_port = cap_x86_64_io_port_last_port (slot->cap);

  if (port < first_port || port + size > last_port)
    return ipc_range_error (first_port, last_port);

  return no_error;
}

error_t
x86_64_io_port_in8 (cte_t *slot, word_t port)
{
  error_t err;
  if ((err = x86_64_io_port_range_check (slot, port, 1)))
    return err;

  set_mr (0, read_port_b (port));
  return ipc_ok (1);
}

error_t
x86_64_io_port_in16 (cte_t *slot, word_t port)
{
  error_t err;
  if ((err = x86_64_io_port_range_check (slot, port, 1)))
    return err;

  set_mr (0, read_port_w (port));
  return ipc_ok (1);
}

error_t
x86_64_io_port_in32 (cte_t *slot, word_t port)
{
  error_t err;
  if ((err = x86_64_io_port_range_check (slot, port, 1)))
    return err;

  set_mr (0, read_port_l (port));
  return ipc_ok (1);
}

error_t
x86_64_io_port_out8 (cte_t *slot, word_t port, word_t value)
{
  error_t err;
  if ((err = x86_64_io_port_range_check (slot, port, 1)))
    return err;

  write_port_b (port, value);
  return ipc_ok (0);
}

error_t
x86_64_io_port_out16 (cte_t *slot, word_t port, word_t value)
{
  error_t err;
  if ((err = x86_64_io_port_range_check (slot, port, 1)))
    return err;

  write_port_w (port, value);
  return ipc_ok (0);
}

error_t
x86_64_io_port_out32 (cte_t *slot, word_t port, word_t value)
{
  error_t err;
  if ((err = x86_64_io_port_range_check (slot, port, 1)))
    return err;

  write_port_l (port, value);
  return ipc_ok (0);
}
