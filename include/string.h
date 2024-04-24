#pragma once

#include <stddef.h>
#include <sys/types.h>

void *memcpy (void *dest, const void *src, size_t n);
void *memmove (void *dest, const void *src, size_t n);
void *memset (void *s, int c, size_t n);
int memcmp (const void *s1, const void *s2, size_t n);

size_t strlen (const char *s);
char *strchr (const char *s, int c);
char *strcpy (char *dest, const char *src);
