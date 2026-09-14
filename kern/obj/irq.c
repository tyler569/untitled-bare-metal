#include "kern/arch.h"
#include "kern/cap.h"
#include "kern/obj/notification.h"
#include "kern/syscall.h"

struct irq_handler_data
{
  int irq;
  bool in_service;
  struct notification *n;
  word_t badge;
};

static uint64_t irq_handlers_issued = 0;
static struct irq_handler_data irq_handlers[16];

message_tag_t
irq_control_get (cte_t *, word_t irq, cte_t *root, word_t index, uint8_t depth)
{
  cte_t *result_cte;
  TRY (lookup_cap_slot (root, index, depth, &result_cte));

  if (cap_type (result_cte) != CAP_NULL)
    return msg_delete_first ();

  if (irq >= 16)
    return msg_invalid_argument (0);

  if ((1 << irq) & irq_handlers_issued)
    return msg_invalid_argument (0);
  irq_handlers_issued |= 1 << irq;

  result_cte->cap = new_irq_handler_cap (irq);
  cap_set_ptr (result_cte, &irq_handlers[irq]);
  irq_handlers[irq].irq = irq;

  return msg_ok (0);
}

message_tag_t
irq_handler_clear (cte_t *obj)
{
  struct irq_handler_data *data = cap_ptr (obj);
  data->n = nullptr;
  return msg_ok (0);
}

message_tag_t
irq_handler_ack (cte_t *obj)
{
  struct irq_handler_data *data = cap_ptr (obj);
  unmask_irq (data->irq);
  data->in_service = false;
  return msg_ok (0);
}

message_tag_t
irq_handler_set_notification (cte_t *obj, cte_t *notification)
{
  struct irq_handler_data *data = cap_ptr (obj);
  struct notification *n = cap_ptr (notification);

  data->n = n;
  data->badge = notification->cap.badge;

  return msg_ok (0);
}

void
handle_irq (word_t irq)
{
  struct irq_handler_data *data = &irq_handlers[irq];

  if (!data->n)
    return;

  if (data->in_service)
    return;

  mask_irq (data->irq);
  data->in_service = true;
  notification_signal (data->n, data->badge);
}
