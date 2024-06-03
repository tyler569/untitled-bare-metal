#pragma once

#include "kern/cap.h"
#include "list.h"
#include "sys/types.h"

struct endpoint
{
  struct list_head list;
};

[[noreturn]] error_t invoke_endpoint_send (cap_t *cap, word_t message_info);
error_t invoke_endpoint_recv (cap_t *cap);
error_t invoke_endpoint_call (cap_t *cap, word_t message_info);
error_t invoke_reply (word_t message_info);
