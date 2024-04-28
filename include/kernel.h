#pragma once

void kernel_main ();
void run_smoke_tests ();

[[noreturn]] void panic (const char *msg, ...);

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))
