#include "string.h"
#include "ctype.h"

void *
memset (void *s, int c, size_t n)
{
  unsigned char *p = s;
  while (n--)
    *p++ = c;
  return s;
}

void *
memcpy (void *dest, const void *src, size_t n)
{
  unsigned char *d = dest;
  const unsigned char *s = src;
  while (n--)
    *d++ = *s++;
  return dest;
}

void *
memmove (void *dest, const void *src, size_t n)
{
  unsigned char *d = dest;
  const unsigned char *s = src;
  if (d < s)
    {
      while (n--)
        *d++ = *s++;
    }
  else
    {
      d += n;
      s += n;
      while (n--)
        *--d = *--s;
    }
  return dest;
}

int
memcmp (const void *s1, const void *s2, size_t n)
{
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  while (n--)
    {
      if (*p1 != *p2)
        return *p1 - *p2;
      p1++;
      p2++;
    }
  return 0;
}

size_t
strlen (const char *s)
{
  size_t len = 0;
  while (*s++)
    len++;
  return len;
}

char *
strchr (const char *s, int c)
{
  while (*s)
    {
      if (*s == c)
        return (char *)s;
      s++;
    }
  return NULL;
}

char *
strcpy (char *dest, const char *src)
{
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

char *
strncpy (char *dest, const char *src, size_t n)
{
  char *d = dest;
  while (n-- && (*d++ = *src++))
    ;
  return dest;
}

int
strcmp (const char *s1, const char *s2)
{
  while (*s1 && *s1 == *s2)
    {
      s1++;
      s2++;
    }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}
