#pragma once
#include "stddef.h"

void kernel_main ();
void run_smoke_tests ();

void create_init_tcb (const void *initrd, size_t size);

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))
