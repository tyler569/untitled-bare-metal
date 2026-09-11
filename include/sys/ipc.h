#pragma once

#include "stdint.h"
#include "sys/types.h"

enum
{
  MESSAGE_MAX_EXTRA_CAPS = 3,
  MESSAGE_MAX_LENGTH = 512 - 5 - MESSAGE_MAX_EXTRA_CAPS,
};

union message_tag_t
{
  struct
  {
    word_t length : 9;
    word_t caps_unwrapped : 3;
    word_t extra_caps : 2;
    word_t label : 50;
  };
  uint64_t value;
};
typedef union message_tag_t message_tag_t;

static inline message_tag_t
message_info_from_word (word_t value)
{
  return (message_tag_t){ .value = value };
}

static inline word_t
message_info_to_word (message_tag_t info)
{
  return info.value;
}

static inline message_tag_t
new_message_tag (word_t label, word_t caps_unwrapped, word_t extra_caps,
                 word_t length)
{
  return (message_tag_t){
    .label = label,
    .caps_unwrapped = caps_unwrapped,
    .extra_caps = extra_caps,
    .length = length,
  };
}

static inline word_t
message_label (message_tag_t tag)
{
  return tag.label;
}

static inline word_t
message_extra_caps (message_tag_t tag)
{
  return tag.extra_caps;
}

static inline word_t
message_caps_unwrapped (message_tag_t tag)
{
  return tag.caps_unwrapped;
}

static inline word_t
message_length (message_tag_t tag)
{
  return tag.length;
}

// The message tag travels in syscall registers, not in this buffer.
struct ipc_buffer
{
  word_t msg[MESSAGE_MAX_LENGTH];
  word_t sender_badge;
  word_t caps_or_badges[MESSAGE_MAX_EXTRA_CAPS];

  cptr_t receive_cnode;
  cptr_t receive_index;
  word_t receive_depth;
};
