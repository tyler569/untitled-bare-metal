#pragma once

#include "kern/cap.h"
#include "list.h"
#include "sys/types.h"

struct endpoint
{
  struct list_head list;
};

[[noreturn]] error_t invoke_endpoint_send (cte_t *cap, word_t message_info);
error_t invoke_endpoint_recv (cte_t *cap);
error_t invoke_endpoint_call (cte_t *cap, word_t message_info);
error_t invoke_reply (cte_t *cap, word_t message_info);
