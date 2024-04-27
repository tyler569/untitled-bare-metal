#pragma once

#include <list.h>
#include <stdint.h>
#include <stdio.h>

void kernel_main ();

[[noreturn]] void panic (const char *msg, ...);

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))

// arch-specific procedures provided by arch code.

struct frame;
typedef struct frame frame_t;

[[noreturn]] void halt_forever ();

void relax_busy_loop ();

ssize_t write_debug (FILE *, const void *str, size_t len);

void print_backtrace (frame_t *);
void print_frame (frame_t *);

void debug_trap ();