#pragma once

#include "kern/cap.h"
#include "kern/size.h"
#include "list.h"
#include "sys/types.h"

struct endpoint
{
  struct list_head list;
};

static_assert (sizeof (struct endpoint) <= BIT (endpoint_size_bits));

void invoke_endpoint_send (cte_t *cap, word_t message_info);
void invoke_endpoint_recv (cte_t *cap);
void invoke_endpoint_nbsend (cte_t *cap, word_t message_info);
void invoke_endpoint_nbrecv (cte_t *cap);

void invoke_endpoint_call (cte_t *cap, word_t message_info);
void invoke_reply (word_t message_info);
void invoke_reply_recv (cte_t *cap, word_t message_info);
