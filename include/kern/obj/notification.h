#pragma once

#include "kern/cap.h"
#include "kern/size.h"
#include "list.h"
#include "sys/types.h"

struct tcb;

struct notification
{
  word_t word;
  struct list_head list;
  struct tcb *bound_tcb;
};

static_assert (sizeof (struct notification) <= BIT (notification_size_bits));

void invoke_notification_send (cte_t *cap);
error_t invoke_notification_recv (cte_t *cap);

void notification_signal (struct notification *n, word_t badge);
