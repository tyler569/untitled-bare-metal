#pragma once

#define USED __attribute__ ((used, unused))
#define MUST_USE __attribute__ ((warn_unused_result))
#define PACKED __attribute__ ((packed))
#define PRINTF_FORMAT(a, b) __attribute__ ((format (printf, a, b)))
#define PURE __attribute__ ((pure))

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_ (x)
#define FILE_AND_LINE FILE_BASENAME ":" STRINGIFY (__LINE__)

#define LIKELY(x) __builtin_expect (!!(x), 1)
#define UNLIKELY(x) __builtin_expect (!!(x), 0)

#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGN_UP(x, a) ALIGN_DOWN ((x) + (a) - 1, a)

#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))

#define MIN(a, b)                                                             \
  ({                                                                          \
    typeof (a) _a = (a);                                                      \
    typeof (b) _b = (b);                                                      \
    _a < _b ? _a : _b;                                                        \
  })

#define MAX(a, b)                                                             \
  ({                                                                          \
    typeof (a) _a = (a);                                                      \
    typeof (b) _b = (b);                                                      \
    _a > _b ? _a : _b;                                                        \
  })

#define CONTAINER_OF(ptr, type, member)                                       \
  ((type *)((uintptr_t)(ptr) - offsetof (type, member)))

#define UNREACHABLE() __builtin_unreachable ()
