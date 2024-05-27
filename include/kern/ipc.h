#pragma once

#include "sys/ipc.h"

word_t get_ipc_info ();
word_t get_ipc_label ();
word_t get_ipc_extra_caps ();
word_t get_ipc_length ();

word_t get_mr (word_t i);
word_t get_extra_cap (word_t i);

void set_mr (word_t i, word_t v);
void set_ipc_info (message_info_t tag);
