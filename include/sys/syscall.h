#pragma once

#include "stdint.h"

struct frame;
typedef struct frame frame_t;

uintptr_t do_syscall (uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t,
                      uintptr_t, int syscall_number, frame_t *);
