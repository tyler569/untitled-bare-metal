set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(triple x86_64-unknown-none)
set(compiler x86_64-elf-gcc)

set(CMAKE_ASM_COMPILER ${compiler})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_C_COMPILER ${compiler})
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_LINKER x86_64-elf-ld)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -mno-red-zone -mno-mmx -mno-3dnow -mno-sse -mno-sse2")
