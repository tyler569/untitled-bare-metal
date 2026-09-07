#pragma once

#include "kern/cap.h"
#include "kern/size.h"
#include "list.h"
#include "sys/types.h"

struct endpoint
{
  struct list_head list;
};

static_assert (sizeof (struct endpoint) <= BIT (ENDPOINT_SIZE_BITS));

// sends
void invoke_endpoint_send (cte_t *cap, message_info_t tag);
void invoke_reply (message_info_t tag);
void invoke_endpoint_nbsend (cte_t *cap, message_info_t tag);

// recvs
message_info_t invoke_endpoint_recv (cte_t *cap);
message_info_t invoke_endpoint_nbrecv (cte_t *cap);

// send + recvs
void invoke_endpoint_call (cte_t *cap, message_info_t tag);
message_info_t invoke_reply_recv (cte_t *cap, message_info_t tag);
