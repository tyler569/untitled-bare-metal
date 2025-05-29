#include "stddef.h"
#include "stdint.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

#include "lib.h"

struct ipc_buffer *__ipc_buffer;
[[noreturn]] USED __attribute__ ((naked)) void
_start ()
{
  asm ("mov %%r15, %0" : "=m"(__ipc_buffer));
  asm ("jmp main");
}

static inline uintptr_t
_syscall0 (int syscall_num)
{
  uintptr_t ret;
  asm volatile ("syscall" : "=a"(ret) : "0"(syscall_num) : "rcx", "r11");
  return ret;
}

static inline uintptr_t
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret)
                : "0"(syscall_num), "D"(a1), "S"(a2)
                : "rcx", "r11");
  return ret;
}

static inline uintptr_t
_syscall22 (int syscall_num, uintptr_t a1, uintptr_t a2, uintptr_t *out)
{
  uintptr_t ret;
  asm volatile ("syscall"
                : "=a"(ret), "=D"(*out)
                : "0"(syscall_num), "1"(a1), "S"(a2)
                : "rcx", "r11");
  return ret;
}

void
send (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_send, cap, info);
}

void
nbsend (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  _syscall2 (sys_nbsend, cap, info);
}

void
signal (cptr_t cap)
{
  _syscall2 (sys_send, cap, 0);
}

message_info_t
call (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_call, cap, info, sender);
}

message_info_t
__call_kernel (cptr_t cap, message_info_t info)
{
  __ipc_buffer->tag = info;
  return _syscall2 (sys_call, cap, info);
}

message_info_t
recv (cptr_t cap, word_t *sender)
{
  return _syscall22 (sys_recv, cap, 0, sender);
}

message_info_t
nbrecv (cptr_t cap, word_t *sender)
{
  return _syscall22 (sys_nbrecv, cap, 0, sender);
}

void
wait (cptr_t cap, word_t *nfn_word)
{
  _syscall22 (sys_recv, cap, 0, nfn_word);
}

message_info_t
reply (message_info_t info)
{
  __ipc_buffer->tag = info;
  return _syscall2 (sys_reply, info, 0);
}

message_info_t
reply_recv (cptr_t cap, message_info_t info, word_t *sender)
{
  __ipc_buffer->tag = info;
  return _syscall22 (sys_replyrecv, cap, info, sender);
}

void
yield ()
{
  _syscall0 (sys_yield);
}

[[noreturn]] void
exit (int)
{
  _syscall0 (0);
  unreachable ();
}

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (sys_debug_write, (uintptr_t)str, len);
  return (long)len;
}

cptr_t
allocate (cptr_t untyped, word_t type, size_t n)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, 0, init_cap_root_cnode, init_cap_root_cnode,
                  64, cptr, n);
  return cptr;
}

cptr_t
allocate_with_size (cptr_t untyped, word_t type, size_t n, uint8_t size_bits)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, size_bits, init_cap_root_cnode,
                  init_cap_root_cnode, 64, cptr, n);
  return cptr;
}

// memory management, mapping and unmapping

int
map_page (cptr_t untyped, cptr_t vspace, cptr_t page, uintptr_t addr)
{
  while (1)
    {
      int err = x86_64_page_map (page, vspace, addr, 0x7);

      if (err != failed_lookup) // including "no_error"
        return err;

      switch (get_mr (0))
        {
        case 3:
          cptr_t pdpt = allocate (untyped, cap_x86_64_pdpt, 1);
          assert (x86_64_pdpt_map (pdpt, vspace, addr, 0x7) == no_error);
          break;
        case 2:
          cptr_t pd = allocate (untyped, cap_x86_64_pd, 1);
          assert (x86_64_pd_map (pd, vspace, addr, 0x7) == no_error);
          break;
        case 1:
          cptr_t pt = allocate (untyped, cap_x86_64_pt, 1);
          assert (x86_64_pt_map (pt, vspace, addr, 0x7) == no_error);
          break;
        default:
          assert (0);
        }
    }
}

buffer_t
create_buffer (cptr_t untyped, size_t pages)
{
  return (buffer_t){ allocate (untyped, cap_x86_64_page, pages), pages };
}

int
map_buffer (cptr_t untyped, cptr_t vspace, buffer_t buffer, uintptr_t addr)
{
  int err;
  for (size_t i = 0; i < buffer.pages; i++)
    {
      err = map_page (untyped, vspace, buffer.cptr_base + i,
                      addr + i * 0x1000);
      if (err != 0)
        return err;
    }

  return no_error;
}

uintptr_t mappable_addr = 0x900000;

uintptr_t
map_buffer_to_mappable_space (cptr_t untyped, cptr_t vspace, buffer_t buffer)
{
  uintptr_t addr = mappable_addr;
  int err = map_buffer (untyped, vspace, buffer, addr);
  mappable_addr += buffer.pages * 0x1000;
  if (err != 0)
    return 0;
  return addr;
}
