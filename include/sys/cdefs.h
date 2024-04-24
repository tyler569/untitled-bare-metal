#pragma once

#define USED __attribute__ ((used, unused))
#define MUST_USE __attribute__ ((warn_unused_result))
#define PACKED __attribute__ ((packed))
#define PRINTF_FORMAT(a, b) __attribute__ ((format (printf, a, b)))
