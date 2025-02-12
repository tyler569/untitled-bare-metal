#pragma once
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "sys/bootinfo.h"
#include "sys/cdefs.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

#define assert(x)                                                             \
  if (!(x))                                                                   \
    {                                                                         \
      printf ("assertion failed: %s\n", #x);                                  \
      unreachable ();                                                         \
    }

extern struct ipc_buffer *__ipc_buffer;

struct frame
{
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
  uint64_t int_no, err_code;
  uint64_t rip, cs, rflags, rsp, ss;
};
typedef struct frame frame_t;

void cptr_alloc_init (struct boot_info *bi);
cptr_t cptr_alloc ();
cptr_t cptr_alloc_range (size_t n);
void cptr_free (cptr_t cptr);

struct buffer
{
  cptr_t cptr_base;
  size_t pages;
};
typedef struct buffer buffer_t;

buffer_t create_buffer (cptr_t untyped, size_t pages);
int map_page (cptr_t untyped, cptr_t vspace, cptr_t page, uintptr_t addr);
int map_buffer (cptr_t untyped, cptr_t vspace, buffer_t buffer,
                uintptr_t addr);
uintptr_t map_buffer_to_mappable_space (cptr_t untyped, cptr_t vspace,
                                        buffer_t buffer);

cptr_t allocate (cptr_t untyped, word_t type, size_t n);
cptr_t allocate_with_size (cptr_t untyped, word_t type, size_t n,
                           uint8_t size_bits);

cptr_t create_process (void *elf_data, size_t elf_size, cptr_t untyped,
                       cptr_t our_vspace);

static inline void
set_mr (word_t i, word_t val)
{
  __ipc_buffer->msg[i] = val;
}

static inline word_t
get_mr (word_t i)
{
  return __ipc_buffer->msg[i];
}

static inline void
set_cap (word_t i, word_t val)
{
  __ipc_buffer->caps_or_badges[i] = val;
}

static inline word_t
get_cap (word_t i)
{
  return __ipc_buffer->caps_or_badges[i];
}

void send (cptr_t cap, message_info_t info);
void signal (cptr_t cap);
message_info_t call (cptr_t cap, message_info_t info, word_t *sender);
message_info_t __call_kernel (cptr_t cap, message_info_t info);
message_info_t recv (cptr_t cap, word_t *sender);
void wait (cptr_t cap, word_t *nfn_word);
message_info_t reply (message_info_t info);
message_info_t reply_recv (cptr_t cap, message_info_t info, word_t *sender);

void yield ();
[[noreturn]] void exit (int);

long write (FILE *, const void *str, unsigned long len);

cptr_t allocate (cptr_t untyped, word_t type, size_t n);

#include "sys/user_method_stubs.h"
