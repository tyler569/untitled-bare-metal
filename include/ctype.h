#pragma once

static inline int
isdigit (int c)
{
  return c >= '0' && c <= '9';
}

static inline int
islower (int c)
{
  return c >= 'a' && c <= 'z';
}

static inline int
isupper (int c)
{
  return c >= 'A' && c <= 'Z';
}

static inline int
isalpha (int c)
{
  return islower (c) || isupper (c);
}

static inline int
isalnum (int c)
{
  return isalpha (c) || isdigit (c);
}

static inline int
isprint (int c)
{
  return c >= 0x20 && c <= 0x7E;
}

static inline int
isspace (int c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'
         || c == '\v';
}

static inline int
isxdigit (int c)
{
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
