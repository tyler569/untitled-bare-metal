#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/obj/notification.h"
#include "kern/syscall.h"

struct irq_handler_data
{
  word_t irq;
  struct notification *n;
  word_t badge;
  bool in_service;
};

static uint64_t irq_handlers_issued = 0;
static struct irq_handler_data irq_handlers[16];

message_info_t
irq_control_get (struct cap *, word_t irq, struct cap *root, word_t index,
                 uint8_t depth)
{
  struct cap *result_cte;
  TRY (lookup_cap_slot (root, index, depth, &result_cte));

  if (cap_type (result_cte) != cap_null)
    return msg_delete_first ();

  if (irq >= 16)
    return msg_invalid_argument (0);

  if ((1 << irq) & irq_handlers_issued)
    return msg_invalid_argument (0);
  irq_handlers_issued |= 1 << irq;

  cap_irq_handler_init (result_cte, irq);
  cap_set_ptr (result_cte, &irq_handlers[irq]);
  irq_handlers[irq].irq = irq;

  return msg_ok (0);
}

message_info_t
irq_handler_clear (struct cap *obj)
{
  struct irq_handler_data *data = cap_ptr (obj);
  data->n = nullptr;
  return msg_ok (0);
}

message_info_t
irq_handler_ack (struct cap *obj)
{
  struct irq_handler_data *data = cap_ptr (obj);
  send_eoi (data->irq);
  data->in_service = false;
  return msg_ok (0);
}

message_info_t
irq_handler_set_notification (struct cap *obj, struct cap *notification)
{
  struct irq_handler_data *data = cap_ptr (obj);
  struct notification *n = cap_ptr (notification);

  data->n = n;
  data->badge = notification->badge;

  return msg_ok (0);
}

bool
handle_irq (word_t irq)
{
  struct irq_handler_data *data = &irq_handlers[irq];

  // Tell the platform code not to EOI since we're waiting until ack()
  // is called.
  if (data->n && data->in_service)
    return true;

  if (data->n && !data->in_service)
    {
      data->in_service = true;
      notification_signal (data->n, data->badge);
      return true;
    }

  return false;
}
