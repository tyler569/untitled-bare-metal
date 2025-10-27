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

// sends
void invoke_endpoint_send (struct cap *cap);
void invoke_reply ();
void invoke_endpoint_nbsend (struct cap *cap);

// recvs
message_info_t invoke_endpoint_recv (struct cap *cap);
message_info_t invoke_endpoint_nbrecv (struct cap *cap);

// send + recvs
void invoke_endpoint_call (struct cap *cap);
message_info_t invoke_reply_recv (struct cap *cap);
