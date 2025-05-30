include_directories(.)

set(archive_dir "${CMAKE_BINARY_DIR}/initrd")
set(archive_name "${CMAKE_BINARY_DIR}/initrd.tar")
set(archive_binaries)

add_library(userlib STATIC
  lib/cptr_alloc.c
  lib/exec.c
  lib/lib.c
  lib/pci.c
  ../lib/num.c
  ../lib/print.c
  ../lib/sort.c
  ../lib/string.c
  ../lib/tar.c
)

add_single_file_program(serial_driver)

add_subdirectory_program(calculator)
add_subdirectory_program(main)

find_program(tar_command NAMES gtar tar)

if (NOT tar_command)
  message(FATAL_ERROR "GNU tar (gtar) not found. Please install it (e.g., via Homebrew: brew install gnu-tar).")
endif()

# Run a check to see if it's actually GNU tar
execute_process(
  COMMAND ${tar_command} --version
  OUTPUT_VARIABLE tar_version_output
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if (NOT tar_version_output MATCHES "GNU tar")
  message(FATAL_ERROR "Found 'tar' is not GNU tar. Please install GNU tar (e.g., via Homebrew: brew install gnu-tar) and ensure it's in your PATH.")
endif()

add_custom_command(
  DEPENDS ${archive_binaries}
  OUTPUT ${archive_name}
  COMMAND ${tar_command} -cf ${archive_name} --transform='s|.*/||'
    ${archive_binaries}
)
add_custom_target(initrd_tarball DEPENDS ${archive_name})
set(initrd_file_path ${archive_name} PARENT_SCOPE)
