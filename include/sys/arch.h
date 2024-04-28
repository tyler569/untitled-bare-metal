#pragma once

#include "stdint.h"
#include "sys/types.h"

#define PAGE_SIZE 4096

struct frame;
typedef struct frame frame_t;

void halt_until_interrupt ();
[[noreturn]] void halt_forever ();

void relax_busy_loop ();

struct stream;
ssize_t write_debug (struct stream *, const void *str, size_t len);

void print_backtrace (frame_t *);
void print_frame (frame_t *);

void debug_trap ();

bool get_initrd_info (void **initrd_start, size_t *initrd_size);

void jump_to_userland (uintptr_t entry, uintptr_t stack);