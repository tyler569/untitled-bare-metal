#pragma once

#include "string.h"
#include <stdatomic.h>

enum
{
  serial_null_cap,
  serial_tcb_cap,
  serial_cnode_cap,
  serial_serial_port_cap,
  serial_endpoint_cap,
  serial_notification_cap,
  serial_irq_cap,
  serial_irq_notification_cap,
  serial_broker_notification_cap,
  serial_broker_endpoint_cap,
};

enum
{
  serial_driver_write = 1,
  serial_driver_read = 2,
};

enum
{
  role_serial_driver,
  role_serial_broker,
};

struct serial_driver_ring
{
  atomic_ushort head;
  atomic_ushort tail;
  char buffer[];
};
typedef struct serial_driver_ring sdr_t;
constexpr size_t serial_driver_ring_size = 4096 - sizeof (sdr_t);
sdr_t *const shared_ring = (sdr_t *)0x1200000;

// Helper: Compute the number of bytes available to read.
static inline unsigned short
sdr_available_read (const sdr_t *rb, unsigned short head, unsigned short tail)
{
  (void)rb;
  if (head >= tail)
    return head - tail;
  else
    return serial_driver_ring_size - tail + head;
}

// Helper: Compute the free space available for writing.
// One slot is reserved to distinguish full vs. empty.
static inline unsigned short
sdr_available_write (const sdr_t *rb, unsigned short head, unsigned short tail)
{
  (void)rb;
  return serial_driver_ring_size - 1 - sdr_available_read (rb, head, tail);
}

static inline bool
serial_driver_ring_empty (const sdr_t *rb)
{
  unsigned short head = atomic_load_explicit (&rb->head, memory_order_relaxed);
  unsigned short tail = atomic_load_explicit (&rb->tail, memory_order_acquire);

  unsigned short available = sdr_available_read (rb, head, tail);
  return available == 0;
}

///
/// Write up to `count` bytes from `data` into the ring buffer.
/// Returns the number of bytes actually written.
///
static inline size_t
serial_driver_ring_write (sdr_t *rb, const char *data, size_t count)
{
  unsigned short head = atomic_load_explicit (&rb->head, memory_order_relaxed);
  unsigned short tail = atomic_load_explicit (&rb->tail, memory_order_acquire);

  unsigned short free_space = sdr_available_write (rb, head, tail);
  if (free_space == 0)
    return 0;

  size_t to_write = (count < free_space) ? count : free_space;

  // Determine how many bytes can be written before wrapping around.
  size_t first_chunk = serial_driver_ring_size - head;
  if (first_chunk > to_write)
    first_chunk = to_write;

  memcpy (&rb->buffer[head], data, first_chunk);

  // If the write wraps around, copy the remaining bytes at the beginning.
  if (to_write > first_chunk)
    {
      memcpy (rb->buffer, data + first_chunk, to_write - first_chunk);
    }

  // Compute the new head index (wrap-around modulo serial_driver_ring_size).
  unsigned short new_head = (head + to_write) % serial_driver_ring_size;
  atomic_store_explicit (&rb->head, new_head, memory_order_release);

  return to_write;
}

///
/// Read up to `count` bytes from the ring buffer into `data`.
/// Returns the number of bytes actually read.
///
static inline size_t
serial_driver_ring_read (sdr_t *rb, char *data, size_t count)
{
  unsigned short tail = atomic_load_explicit (&rb->tail, memory_order_relaxed);
  unsigned short head = atomic_load_explicit (&rb->head, memory_order_acquire);

  unsigned short available = sdr_available_read (rb, head, tail);
  if (available == 0)
    return 0;

  size_t to_read = (count < available) ? count : available;

  // Determine how many bytes can be read before wrapping around.
  size_t first_chunk = serial_driver_ring_size - tail;
  if (first_chunk > to_read)
    first_chunk = to_read;

  memcpy (data, &rb->buffer[tail], first_chunk);

  // If the read wraps around, copy the remaining bytes from the beginning.
  if (to_read > first_chunk)
    {
      memcpy (data + first_chunk, rb->buffer, to_read - first_chunk);
    }

  // Compute the new tail index.
  unsigned short new_tail = (tail + to_read) % serial_driver_ring_size;
  atomic_store_explicit (&rb->tail, new_tail, memory_order_release);

  return to_read;
}
