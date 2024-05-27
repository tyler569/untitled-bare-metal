#pragma once

void kernel_main ();
void run_smoke_tests ();

void create_init_tcb (void *elf_header);

#define volatile_read(x) (*(volatile typeof (x) *)&(x))
#define volatile_write(x, y) ((*(volatile typeof (x) *)&(x)) = (y))
