#pragma once

#include "list.h"
#include "stddef.h"
#include "stdint.h"
#include "sys/types.h"

void kernel_main ();

[[noreturn]] void panic (const char *msg, ...);

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))

// arch-specific procedures provided by arch code.

struct frame;
typedef struct frame frame_t;

[[noreturn]] void halt_forever ();

void relax_busy_loop ();

struct stream;
ssize_t write_debug (struct stream *, const void *str, size_t len);

void print_backtrace (frame_t *);
void print_frame (frame_t *);

void debug_trap ();