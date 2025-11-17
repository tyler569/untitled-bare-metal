set(targets_to_archive userland calculator_server serial_driver pci_manager cdt_test)
set(archive_dir "${CMAKE_BINARY_DIR}/user/initrd")
set(archive_file_paths)

foreach (program ${targets_to_archive})
  add_executable(${program}
    ${program}.c
    cptr_alloc.c
    exec.c
    lib.c
    pci.c
    ../lib/num.c
    ../lib/print.c
    ../lib/sort.c
    ../lib/string.c
    ../lib/tar.c
  )
  list(APPEND archive_file_paths $<TARGET_FILE:${program}>)
endforeach ()

find_program(tar_command NAMES gtar tar REQUIRED)
if (NOT tar_command)
  message(FATAL_ERROR "tar command not found")
endif()

add_custom_command(
  OUTPUT ${archive_name}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${archive_dir}
  COMMAND ${CMAKE_COMMAND} -E copy ${archive_file_paths} ${archive_dir}
  COMMAND ${CMAKE_COMMAND} -E chdir ${archive_dir}
          ${tar_command} -cf ${archive_name} ${targets_to_archive}
  DEPENDS ${targets_to_archive}
  VERBATIM
)
add_custom_target(initrd_tarball DEPENDS ${archive_name})

