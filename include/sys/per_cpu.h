#pragma once

#include "list.h"

#ifdef __x86_64__
#include "arch/x86_64/exports.h"
#elifdef __aarch64__
#include "arch/arm64/exports.h"
#endif

struct per_cpu
{
  struct per_cpu *self;
  struct arch_per_cpu arch;
  uintptr_t kernel_stack_top;
  struct task *current_task;
  struct list_head list;
  bool printing_backtrace;
};

typedef struct per_cpu per_cpu_t;
