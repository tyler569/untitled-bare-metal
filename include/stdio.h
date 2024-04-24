#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/types.h>

struct stream;
typedef struct stream FILE;

int print (const char *str);
int puts (const char *str);

int printf (const char *format, ...) PRINTF_FORMAT (1, 2);
int fprintf (FILE *file, const char *format, ...) PRINTF_FORMAT (2, 3);
int fnprintf (FILE *file, size_t len, const char *format, ...)
    PRINTF_FORMAT (3, 4);

int vprintf (const char *format, va_list args);
int vfprintf (FILE *file, const char *format, va_list args);
int vfnprintf (FILE *file, size_t len, const char *format, va_list args);
