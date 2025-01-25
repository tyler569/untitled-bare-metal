#pragma once

static inline bool
isdigit (int c)
{
  return c >= '0' && c <= '9';
}

static inline bool
islower (int c)
{
  return c >= 'a' && c <= 'z';
}

static inline bool
isupper (int c)
{
  return c >= 'A' && c <= 'Z';
}

static inline bool
isalpha (int c)
{
  return islower (c) || isupper (c);
}

static inline bool
isalnum (int c)
{
  return isalpha (c) || isdigit (c);
}

static inline bool
isprint (int c)
{
  return c >= 0x20 && c <= 0x7E;
}

static inline bool
isspace (int c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'
         || c == '\v';
}

static inline bool
isxdigit (int c)
{
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
