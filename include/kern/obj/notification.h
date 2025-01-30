#pragma once

#include "kern/cap.h"
#include "list.h"
#include "sys/types.h"

struct tcb;

struct notification
{
  word_t word;
  struct list_head list;
  struct tcb *bound_tcb;
};

void invoke_notification_send (cte_t *cap);
message_info_t invoke_notification_recv (cte_t *cap, word_t *nfn_word);

void notification_signal (struct notification *n, word_t badge);
