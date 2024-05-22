#pragma once

#include "stddef.h"
#include "stdint.h"
#include "sys/cdefs.h"

struct arch_per_cpu {};

static inline void *get_tls () {
    void *tls;
    asm volatile ("mrs %0, tpidr_el0" : "=r" (tls));
    return tls;
}

static inline void set_tls (void *tls) {
    asm volatile ("msr tpidr_el0, %0" : : "r" (tls));
}

static inline void *get_cpu () {
    return get_tls ();
}

#define this_cpu ((per_cpu_t *) get_cpu ())
