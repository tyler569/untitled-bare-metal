#pragma once

#include "sys/types.h"

struct frame;
typedef struct frame frame_t;

[[noreturn]] void halt_forever ();

void relax_busy_loop ();

struct stream;
ssize_t write_debug (struct stream *, const void *str, size_t len);

void print_backtrace (frame_t *);
void print_frame (frame_t *);

void debug_trap ();
