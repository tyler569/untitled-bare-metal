#pragma once

#include "kern/cap.h"
#include "list.h"
#include "sys/types.h"

struct notification
{
  word_t word;
  struct list_head list;
};

[[noreturn]] void invoke_notification_send (cte_t *cap);
message_info_t invoke_notification_recv (cte_t *cap, word_t *nfn_word);
