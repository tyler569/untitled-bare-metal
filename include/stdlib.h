#pragma once

#include "stddef.h"

void qsort (void *base, size_t nmemb, size_t size,
            int (*compar) (const void *, const void *));

long strtol (const char *nptr, char **endptr, int base);
