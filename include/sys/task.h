#pragma once

#include "list.h"
#include "sys/cdefs.h"

struct task
{
  struct list_head tasks;
  struct list_head children;
  struct elf_file *elf;
  struct frame *frame;
};
