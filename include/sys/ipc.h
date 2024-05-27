#pragma once

#include "stdint.h"
#include "sys/types.h"

enum
{
  MESSAGE_MAX_EXTRA_CAPS = 4,
  MESSAGE_MAX_LENGTH = 512 - 5 - MESSAGE_MAX_EXTRA_CAPS,
};

enum object_method_bases
{
  tcb_methods = 0x1000,
  frame_methods = 0x2000,
  pml4_methods = 0x3000,
  pdpt_methods = 0x4000,
  pd_methods = 0x5000,
  pt_methods = 0x6000,
  cnode_methods = 0x7000,
  untyped_methods = 0x8000,
};

static inline message_info_t
new_message_info (word_t label, word_t extra_caps, word_t length)
{
  return (label << 12) | (extra_caps << 10) | length;
}

static inline word_t
get_message_label (message_info_t tag)
{
  return tag >> 12;
}

static inline word_t
get_message_extra_caps (message_info_t tag)
{
  return (tag >> 10) & 0x3;
}

static inline word_t
get_message_length (message_info_t tag)
{
  return tag & 0x3ff;
}

struct ipc_buffer
{
  message_info_t tag;

  word_t msg[MESSAGE_MAX_LENGTH];
  word_t user_data;
  word_t caps_or_badges[MESSAGE_MAX_EXTRA_CAPS];

  cptr_t receive_cnode;
  cptr_t receive_index;
  word_t receive_depth;
};


