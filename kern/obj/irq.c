#include "kern/cap.h"
#include "kern/obj/notification.h"
#include "kern/syscall.h"

struct irq_handler_data
{
  struct notification *n;
  word_t badge;
  bool in_service;
};

static uint64_t irq_handlers_issued = 0;
static struct irq_handler_data irq_handlers[16];

message_info_t
irq_control_get (cte_t *, word_t irq, cte_t *root, word_t index, uint8_t depth)
{
  error_t err;
  cte_t *result_cte = lookup_cap_slot (root, index, depth, &err);
  if (err != no_error)
    return return_ipc (err, 0);

  if (cap_type (result_cte) != cap_null)
    return return_ipc (delete_first, 0);

  if (irq >= 16)
    return return_ipc (invalid_argument, 0);

  if ((1 << irq) & irq_handlers_issued)
    return return_ipc (invalid_argument, 0);
  irq_handlers_issued |= 1 << irq;

  result_cte->cap = cap_irq_handler_new (irq);
  cap_set_ptr (result_cte, &irq_handlers[irq]);

  return return_ipc (no_error, 0);
}

message_info_t
irq_handler_clear (cte_t *obj)
{
  struct irq_handler_data *data = cap_ptr (obj);
  data->n = nullptr;
  return return_ipc (no_error, 0);
}

message_info_t
irq_handler_ack (cte_t *obj)
{
  struct irq_handler_data *data = cap_ptr (obj);
  data->in_service = false;
  return return_ipc (no_error, 0);
}

message_info_t
irq_handler_set_notification (cte_t *obj, cte_t *notification)
{
  struct irq_handler_data *data = cap_ptr (obj);
  struct notification *n = cap_ptr (notification);

  data->n = n;
  data->badge = notification->cap.badge;

  return return_ipc (no_error, 0);
}

void
handle_irq (word_t irq)
{
  struct irq_handler_data *data = &irq_handlers[irq];
  if (data->n && !data->in_service)
    {
      data->in_service = true;
      notification_signal (data->n, data->badge);
    }
}
