cmake_minimum_required(VERSION 3.27)
project(untitled_bare_metal C ASM)

set(CMAKE_C_STANDARD 23)

include(ExternalProject)

add_compile_options(-Wall -Wextra -mcmodel=kernel)

add_link_options(
  -T ${CMAKE_SOURCE_DIR}/link.ld
  -z max-page-size=0x1000
)

include_directories(include)

set(CMAKE_C_FLAGS_DEBUG "-g -O0")

# Helper function to add preprocessor definition of FILE_BASENAME
# to pass the filename without directory path for debugging use.
#
# Note that in header files this is not consistent with
# __FILE__ and __LINE__ since FILE_BASENAME will be the
# compilation unit source file name (.c/.cpp).
#
# Example:
#
#   define_file_basename_for_sources(my_target)
#
# Will add -DFILE_BASENAME="filename" for each source file depended on
# by my_target, where filename is the name of the file.
#
function(define_file_basename_for_sources targetname)
  get_target_property(source_files "${targetname}" SOURCES)
  foreach (sourcefile ${source_files})
    # Add the FILE_BASENAME=filename compile definition to the list.
    get_filename_component(basename "${sourcefile}" NAME)
    # Set the updated compile definitions on the source file.
    set_property(
      SOURCE "${sourcefile}" APPEND
      PROPERTY COMPILE_DEFINITIONS "FILE_BASENAME=\"${basename}\"")
  endforeach ()
endfunction()

add_executable(untitled_bare_metal
  main.c
  arch/x86_64/entry.S
  arch/x86_64/idt.c
  arch/x86_64/intrin.c
  arch/x86_64/gdt.c
  arch/x86_64/page_alloc.c
  arch/x86_64/entry.c
  lib/print.c
  lib/string.c
  arch/x86_64/smp.c
  lib/spin_lock.c
  include/sys/lock.h
  arch/x86_64/lapic.c
  debug.c
  arch/x86_64/ints.c
  arch/x86_64/mmu.c
  lib/slab.c
)

define_file_basename_for_sources(untitled_bare_metal)

install(TARGETS untitled_bare_metal DESTINATION bin)
install(DIRECTORY include DESTINATION include)

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
    DOWNLOAD_COMMAND git clone --depth 1 --branch v4.x-branch-binary https://github.com/limine-bootloader/limine.git ${limine_dir}
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
  )
endif ()

add_custom_command(
  DEPENDS untitled_bare_metal limine
  COMMAND cp ${CMAKE_SOURCE_DIR}/limine.cfg ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine.sys ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-cd.bin ${iso_dir}/boot/limine
  COMMAND cp ${limine_dir}/limine-cd-efi.bin ${iso_dir}/boot/limine
  COMMAND cp ${CMAKE_BINARY_DIR}/untitled_bare_metal ${iso_dir}/boot
  COMMAND xorriso -as mkisofs -b boot/limine/limine-cd.bin -no-emul-boot -boot-load-size 4 --boot-info-table --efi-boot boot/limine/limine-cd-efi.bin -efi-boot-part --efi-boot-image --protective-msdos-label "${iso_dir}" -o "${iso_file}"
  COMMAND ${limine_dir}/limine-deploy ${iso_file}
  OUTPUT ${iso_file}
)

add_custom_target(iso ALL DEPENDS ${iso_file})