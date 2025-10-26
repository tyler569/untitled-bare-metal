#pragma once

#include "stdint.h"
#include "sys/types.h"

enum
{
  MESSAGE_MAX_EXTRA_CAPS = 4,
  MESSAGE_MAX_LENGTH = 128 - 5 - MESSAGE_MAX_EXTRA_CAPS,
};

union message_info_t
{
  struct
    { 
      word_t length : 8;
      word_t caps_unwrapped : 3;
      word_t extra_caps : 2;
      word_t label : 51;
    };
  uint64_t value;
};
typedef union message_info_t message_info_t;

static inline message_info_t
new_message_info (word_t label, word_t caps_unwrapped, word_t extra_caps,
                  word_t length)
{
  return (message_info_t){
      .label = label,
      .caps_unwrapped = caps_unwrapped,
      .extra_caps = extra_caps,
      .length = length,
  };
}

static inline word_t
get_message_label (message_info_t tag)
{
  return tag.label;
}

static inline word_t
get_message_extra_caps (message_info_t tag)
{
  return tag.extra_caps;
}

static inline word_t
get_message_caps_unwrapped (message_info_t tag)
{
  return tag.caps_unwrapped;
}

static inline word_t
get_message_length (message_info_t tag)
{
  return tag.length;
}

struct ipc_buffer
{
  message_info_t tag;

  word_t msg[MESSAGE_MAX_LENGTH];
  word_t sender_badge;
  word_t caps_or_badges[MESSAGE_MAX_EXTRA_CAPS];

  cptr_t receive_cnode;
  cptr_t receive_index;
  word_t receive_depth;
};
