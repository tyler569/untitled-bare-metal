cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalProject)
include(DefineFileBasenameForSources)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS_DEBUG "-g -O1")
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

add_compile_options(-Wall -Wextra -Wnewline-eof -fbuiltin)
add_compile_options(-iquote ${CMAKE_SOURCE_DIR}/include)

add_compile_definitions(__UNTITLED_BARE_METAL__=1)

add_executable(userland
  user/userland.c
  user/exec.c
  user/cptr_alloc.c
  lib/print.c
  lib/string.c
)
define_file_basename_for_sources(userland)

add_subdirectory(kern)

add_custom_command(
  DEPENDS limine untitled_bare_metal userland "${CMAKE_SOURCE_DIR}/limine.cfg"
  OUTPUT ${iso_file}
  COMMAND cp ${CMAKE_SOURCE_DIR}/limine.cfg ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-bios.sys ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-bios-cd.bin ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-uefi-cd.bin ${iso_dir}/boot/limine
  COMMAND cp ${CMAKE_BINARY_DIR}/kern/untitled_bare_metal ${iso_dir}/boot
  COMMAND cp ${CMAKE_BINARY_DIR}/userland ${iso_dir}/boot
  COMMAND xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin
  -no-emul-boot -boot-load-size 4 --boot-info-table
  --efi-boot boot/limine/limine-uefi-cd.bin
  -efi-boot-part --efi-boot-image --protective-msdos-label
  "${iso_dir}" -o "${iso_file}"
  COMMAND ${limine_dir}/limine bios-install ${iso_file}
)
add_custom_target(iso ALL DEPENDS ${iso_file})
