cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalProject)
include(DefineFileBasenameForSources)
include(IDL)
include(UserPrograms)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O1 -std=gnu23")
set(limine_dir "${CMAKE_SOURCE_DIR}/limine")
set(iso_file "${CMAKE_SOURCE_DIR}/untitled_bare_metal.iso")
set(iso_dir "${CMAKE_BINARY_DIR}/isodir")
file(MAKE_DIRECTORY ${iso_dir}/boot/limine)

# We do this as a manual git download as a workaround for the fact that ExternalProject_Add
# doesn't support shallow clones, or more accurately lies about its support for shallow clones.
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/17770
if (NOT EXISTS ${limine_dir})
  ExternalProject_Add(limine
    SOURCE_DIR "${limine_dir}"
    DOWNLOAD_COMMAND git clone --depth 1 --branch v7.x-binary https://github.com/limine-bootloader/limine.git ${limine_dir}
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
  )
endif ()

project(untitled_bare_metal C ASM)

add_compile_options(-Wall -Wextra -fbuiltin)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/limine)

add_compile_definitions(__UNTITLED_BARE_METAL__=1)

set(targets_to_archive calculator_server serial_driver serial_broker)
set(archive_dir "${CMAKE_BINARY_DIR}/initrd")
set(archive_name "${CMAKE_BINARY_DIR}/initrd.tar")
set(archive_binaries)

foreach(program ${targets_to_archive})
  add_single_file_program(${program})
endforeach()

add_subdirectory_program(calculator2)
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

add_subdirectory(kern)

add_custom_command(
  DEPENDS limine untitled_bare_metal ${archive_name} "${CMAKE_SOURCE_DIR}/limine.cfg"
  OUTPUT ${iso_file}
  COMMAND cp ${CMAKE_SOURCE_DIR}/limine.cfg ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-bios.sys ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-bios-cd.bin ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-uefi-cd.bin ${iso_dir}/boot/limine
  COMMAND cp ${CMAKE_BINARY_DIR}/kern/untitled_bare_metal ${iso_dir}/boot
  COMMAND cp ${archive_name} ${iso_dir}/boot
  COMMAND xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin
  -no-emul-boot -boot-load-size 4 --boot-info-table
  --efi-boot boot/limine/limine-uefi-cd.bin
  -efi-boot-part --efi-boot-image --protective-msdos-label
  "${iso_dir}" -o "${iso_file}"
  COMMAND ${limine_dir}/limine bios-install ${iso_file}
)
add_custom_target(iso ALL DEPENDS ${iso_file})
