#pragma once

#include "stddef.h"
#include "stdint.h"

typedef long ssize_t;

// typedef uint64_t message_info_t;
typedef uint64_t word_t;
typedef uint64_t cptr_t;
typedef uint64_t badge_t;
typedef uint64_t exception_t;
typedef uint64_t error_t;

typedef uint64_t x86_vm_attributes_t;
typedef uint64_t cap_rights_t;

struct frame;
typedef struct frame frame_t;
typedef struct frame user_context_t;

constexpr cap_rights_t cap_rights_none = 0;
constexpr cap_rights_t cap_rights_all = 0xFF;
