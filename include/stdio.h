#pragma once

#include "stdarg.h"
#include "stddef.h"
#include "sys/cdefs.h"
#include "sys/types.h"

struct stream;

#ifndef UBM_HOSTED
typedef struct stream FILE;
#endif

int print (const char *str);
int puts (const char *str);

int printf (const char *format, ...) PRINTF_FORMAT (1, 2);
int fprintf (struct stream *file, const char *format, ...)
    PRINTF_FORMAT (2, 3);
int fnprintf (struct stream *file, size_t len, const char *format, ...)
    PRINTF_FORMAT (3, 4);

int vprintf (const char *format, va_list args);
int vfprintf (struct stream *file, const char *format, va_list args);
int vfnprintf (struct stream *file, size_t len, const char *format,
               va_list args);

#ifdef DEBUG
#define debug_printf(...) printf (__VA_ARGS__)
#else
#define debug_printf(...) ((void)0)
#endif
