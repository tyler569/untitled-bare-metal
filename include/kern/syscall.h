#pragma once

#include "sys/syscall.h"
#include "sys/types.h"

struct frame;
typedef struct frame frame_t;

uintptr_t do_syscall (uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t,
                      uintptr_t, int syscall_number, frame_t *);

void return_ipc_error (word_t err, word_t registers);
