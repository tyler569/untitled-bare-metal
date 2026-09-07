#include "stddef.h"
#include "stdint.h"
#include "sys/ipc.h"
#include "sys/syscall.h"
#include "sys/types.h"

#include "lib.h"

// IPC ABI: RAX = syscall number / returned tag, RDI = capability,
// RSI = outgoing tag. Payload words and sender badges stay in the IPC buffer.
static inline word_t
_syscall2 (int syscall_num, uintptr_t a1, uintptr_t a2)
{
  word_t result = syscall_num;
  asm volatile ("syscall"
                : "+a"(result)
                : "D"(a1), "S"(a2)
                : "rcx", "r11", "memory");
  return result;
}

static inline word_t
_syscall0 (int syscall_num)
{
  return _syscall2 (syscall_num, 0, 0);
}

static inline word_t
_syscall1 (int syscall_num, uintptr_t a1)
{
  return _syscall2 (syscall_num, a1, 0);
}

void
send (cptr_t cap, message_info_t info)
{
  _syscall2 (SYS_SEND, cap, message_info_to_word (info));
}

void
nbsend (cptr_t cap, message_info_t info)
{
  _syscall2 (SYS_NBSEND, cap, message_info_to_word (info));
}

void
signal (cptr_t cap)
{
  _syscall1 (SYS_SEND, cap);
}

message_info_t
call (cptr_t cap, message_info_t info, word_t *sender)
{
  info = message_info_from_word (
      _syscall2 (SYS_CALL, cap, message_info_to_word (info)));
  if (sender)
    *sender = __ipc_buffer->sender_badge;
  return info;
}

message_info_t
recv (cptr_t cap, word_t *sender)
{
  message_info_t info = message_info_from_word (_syscall1 (SYS_RECV, cap));
  if (sender)
    *sender = __ipc_buffer->sender_badge;
  return info;
}

message_info_t
nbrecv (cptr_t cap, word_t *sender)
{
  message_info_t info = message_info_from_word (_syscall1 (SYS_NBRECV, cap));
  if (sender)
    *sender = __ipc_buffer->sender_badge;
  return info;
}

void
wait (cptr_t cap, word_t *nfn_word)
{
  _syscall1 (SYS_RECV, cap);
  if (nfn_word)
    *nfn_word = __ipc_buffer->sender_badge;
}

message_info_t
reply (message_info_t info)
{
  return message_info_from_word (
      _syscall2 (SYS_REPLY, 0, message_info_to_word (info)));
}

message_info_t
reply_recv (cptr_t cap, message_info_t info, word_t *sender)
{
  info = message_info_from_word (
      _syscall2 (SYS_REPLYRECV, cap, message_info_to_word (info)));
  if (sender)
    *sender = __ipc_buffer->sender_badge;
  return info;
}

void
yield ()
{
  _syscall0 (SYS_YIELD);
}

[[noreturn]] void
exit (int)
{
  _syscall0 (0);
  unreachable ();
}

[[noreturn]] void
panic (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
  exit (1);
}

long
write (FILE *, const void *str, unsigned long len)
{
  _syscall2 (SYS_DEBUG_WRITE, (uintptr_t)str, len);
  return (long)len;
}

cptr_t
allocate (cptr_t untyped, word_t type, size_t n)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, 0, INIT_CAP_ROOT_CNODE, INIT_CAP_ROOT_CNODE,
                  64, cptr, n);
  return cptr;
}

cptr_t
allocate_with_size (cptr_t untyped, word_t type, size_t n, uint8_t size_bits)
{
  cptr_t cptr = cptr_alloc_range (n);
  untyped_retype (untyped, type, size_bits, INIT_CAP_ROOT_CNODE,
                  INIT_CAP_ROOT_CNODE, 64, cptr, n);
  return cptr;
}

// memory management, mapping and unmapping

int
map_page (cptr_t untyped, cptr_t vspace, cptr_t page, uintptr_t addr)
{
  while (1)
    {
      int err = x86_64_page_map (page, vspace, addr, 0x7);

      if (err != FAILED_LOOKUP) // including "no_error"
        return err;

      switch (get_mr (0))
        {
        case 3:
          cptr_t pdpt = allocate (untyped, CAP_X86_64_PDPT, 1);
          assert (x86_64_pdpt_map (pdpt, vspace, addr, 0x7) == NO_ERROR);
          break;
        case 2:
          cptr_t pd = allocate (untyped, CAP_X86_64_PD, 1);
          assert (x86_64_pd_map (pd, vspace, addr, 0x7) == NO_ERROR);
          break;
        case 1:
          cptr_t pt = allocate (untyped, CAP_X86_64_PT, 1);
          assert (x86_64_pt_map (pt, vspace, addr, 0x7) == NO_ERROR);
          break;
        default:
          assert (0);
        }
    }
}

buffer_t
create_buffer (cptr_t untyped, size_t pages)
{
  return (buffer_t){ allocate (untyped, CAP_X86_64_PAGE, pages), pages };
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

  return NO_ERROR;
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
