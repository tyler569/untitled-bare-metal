#include "tern/ipc.hpp"

static ipc_buffer buffer{};
extern "C"
{
  ipc_buffer *__ipc_buffer = &buffer;
}
static message_info_t response;
static unsigned calls;
static word_t payload;

extern "C" [[noreturn]] void
panic (const char *, ...)
{
  __builtin_trap ();
}
extern "C" message_info_t
call (cptr_t cap, message_info_t request, word_t *badge)
{
  assert (cap == 7);
  assert (message_label (request) == 4);
  assert (message_length (request) == 2);
  assert (get_mr (0) == 10 && get_mr (1) == 20);
  ++calls;
  *badge = 123;
  set_mr (0, payload);
  return response;
}

extern "C" int
main ()
{
  tern::endpoint endpoint{ 7 };
  const tern::message request{ 4, 10, 20 };
  response = new_message_info (4, 0, 0, 1);
  payload = 30;
  auto saved = endpoint.call (request);
  assert (saved && saved->size () == 1 && saved->word (0) == 30);
  assert (saved.badge () == 123);
  payload = 99;
  auto next = endpoint.call (request);
  assert (next && next->word (0) == 99 && saved->word (0) == 30);
  response = new_message_info (5, 0, 0, 0);
  auto error = endpoint.call (request);
  assert (!error && error.error () == tern::ipc_error::unexpected_label);
  assert (error.reply_label () == 5);
  auto alternate = endpoint.call (request, 5);
  assert (alternate && alternate->size () == 0);
  response = new_message_info (4, 0, 0, 17);
  auto oversized = endpoint.call (request);
  assert (!oversized && oversized.error () == tern::ipc_error::invalid_reply);
  response = new_message_info (4, 0, 1, 0);
  auto caps = endpoint.call (request);
  assert (!caps && caps.error () == tern::ipc_error::invalid_reply);
  response = new_message_info (4, 1, 0, 0);
  auto unwrapped = endpoint.call (request);
  assert (!unwrapped && unwrapped.error () == tern::ipc_error::invalid_reply);
  const auto before = calls;
  auto invalid = endpoint.call (tern::message{ word_t{ 1 } << 51 });
  assert (!invalid && invalid.error () == tern::ipc_error::invalid_request);
  assert (calls == before);
  return 0;
}
