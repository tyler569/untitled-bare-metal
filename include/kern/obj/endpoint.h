#pragma once

#include "kern/cap.h"
#include "list.h"
#include "sys/types.h"

struct endpoint
{
  struct list_head list;
};

[[noreturn]] void invoke_endpoint_send (cte_t *cap, word_t message_info);
message_info_t invoke_endpoint_recv (cte_t *cap, word_t *badge);
message_info_t invoke_endpoint_call (cte_t *cap, word_t message_info);
void invoke_reply (word_t message_info);
message_info_t invoke_reply_recv (cte_t *cap, word_t message_info,
                                  word_t *badge);
