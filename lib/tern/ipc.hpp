#pragma once

#include "lib.h"

namespace tern
{
// A deliberately small, allocation-free message subset. The wire length is
// eight bits; the larger backing IPC buffer is not the wire limit.
inline constexpr size_t message_capacity = 16;

class message
{
  word_t label_;
  size_t size_;
  word_t words_[message_capacity]{};

public:
  template <class... Words>
  explicit constexpr
  message (word_t label, Words... words)
      : label_ (label), size_ (sizeof...(Words)),
        words_{ static_cast<word_t> (words)... }
  {
    static_assert (sizeof...(Words) <= message_capacity);
  }
  constexpr word_t
  label () const
  {
    return label_;
  }
  constexpr size_t
  size () const
  {
    return size_;
  }
  word_t
  word (size_t index) const
  {
    assert (index < size_);
    return words_[index];
  }
  friend class endpoint;
};

enum class ipc_error
{
  none,
  invalid_request,
  unexpected_label,
  invalid_reply
};

class [[nodiscard]] reply_result
{
  message message_{ 0 };
  ipc_error error_ = ipc_error::none;
  badge_t badge_ = 0;

public:
  explicit
  operator bool () const
  {
    return error_ == ipc_error::none;
  }
  ipc_error
  error () const
  {
    return error_;
  }
  word_t
  reply_label () const
  {
    return message_.label ();
  }
  badge_t
  badge () const
  {
    return badge_;
  }
  const message *
  operator->() const
  {
    assert (error_ == ipc_error::none);
    return &message_;
  }
  friend class endpoint;
};

// Borrowed capability: copying this value neither copies nor deletes a cap.
class endpoint
{
  cptr_t cap_;

public:
  explicit constexpr
  endpoint (cptr_t cap)
      : cap_ (cap)
  {
  }
  constexpr cptr_t
  native_handle () const
  {
    return cap_;
  }

  // Default protocol convention: successful replies echo the request label.
  // Labels are protocol data, not universally distinguishable kernel errors.
  [[nodiscard]] reply_result
  call (const message &request) const
  {
    return call (request, request.label ());
  }
  [[nodiscard]] reply_result
  call (const message &request, word_t expected_label) const
  {
    reply_result result;
    if (request.label_ >= (word_t{ 1 } << 51))
      {
        result.error_ = ipc_error::invalid_request;
        return result;
      }
    for (size_t i = 0; i < request.size_; ++i)
      set_mr (i, request.words_[i]);
    const auto info
        = ::call (cap_, new_message_info (request.label_, 0, 0, request.size_),
                  &result.badge_);
    result.message_.label_ = message_label (info);
    const auto length = message_length (info);
    if (length > message_capacity || message_extra_caps (info) != 0
        || message_caps_unwrapped (info) != 0)
      {
        result.error_ = ipc_error::invalid_reply;
        return result;
      }
    result.message_.size_ = length;
    for (size_t i = 0; i < length; ++i)
      result.message_.words_[i] = get_mr (i);
    if (result.message_.label_ != expected_label)
      result.error_ = ipc_error::unexpected_label;
    return result;
  }
};
}
