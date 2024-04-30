cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalProject)
include(DefineFileBasenameForSources)

project(untitled_bare_metal C ASM)

set(CMAKE_C_STANDARD 23)

set(CMAKE_C_FLAGS_DEBUG "-g -O1")
add_compile_options(-Wall -Wextra -Wnewline-eof)

add_compile_options(-iquote ${CMAKE_SOURCE_DIR}/include)

set(limine_dir "${CMAKE_SOURCE_DIR}/limine")
set(iso_file "${CMAKE_SOURCE_DIR}/untitled_bare_metal.iso")
set(iso_dir "${CMAKE_BINARY_DIR}/isodir")
file(MAKE_DIRECTORY ${iso_dir}/boot/limine)

add_executable(untitled_bare_metal
  kernel/main.c
  kernel/debug.c
  kernel/page_alloc.c
  kernel/task.c
  lib/chacha20.c
  lib/elf.c
  lib/hexdump.c
  lib/mem_alloc.c
  lib/print.c
  lib/rng.c
  lib/slab_alloc.c
  lib/spin_lock.c
  lib/string.c
  include/arch/x86_64/exports.h
  arch/x86_64/frame.c
  kernel/syscall.c
  include/sys/syscall.h
)

add_executable(userland
  user/userland.c
)

if ("${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "Generic x86_64")
  target_sources(untitled_bare_metal PRIVATE
    arch/x86_64/debug.c
    arch/x86_64/entry.c
    arch/x86_64/isrs.S
    arch/x86_64/gdt.c
    arch/x86_64/idt.c
    arch/x86_64/initrd.c
    arch/x86_64/intrin.c
    arch/x86_64/ints.c
    arch/x86_64/lapic.c
    arch/x86_64/phy_mem.c
    arch/x86_64/mmu.c
    arch/x86_64/smp.c
    arch/x86_64/syscall.c
    arch/x86_64/syscall.S
  )

  target_compile_options(untitled_bare_metal PRIVATE -mcmodel=kernel)
  target_link_options(untitled_bare_metal PRIVATE
    -T ${CMAKE_SOURCE_DIR}/arch/x86_64/link.ld
    -z max-page-size=0x1000
  )
  set_target_properties(untitled_bare_metal
    PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/arch/x86_64/link.ld
  )

  add_custom_command(
    DEPENDS limine untitled_bare_metal userland "${CMAKE_SOURCE_DIR}/limine.cfg"
    OUTPUT ${iso_file}
    COMMAND cp ${CMAKE_SOURCE_DIR}/limine.cfg ${iso_dir}/boot/limine
    COMMAND cp ${limine_dir}/limine-bios.sys ${iso_dir}/boot/limine
    COMMAND cp ${limine_dir}/limine-bios-cd.bin ${iso_dir}/boot/limine
    COMMAND cp ${limine_dir}/limine-uefi-cd.bin ${iso_dir}/boot/limine
    COMMAND cp ${CMAKE_BINARY_DIR}/untitled_bare_metal ${iso_dir}/boot
    COMMAND cp ${CMAKE_BINARY_DIR}/userland ${iso_dir}/boot
    COMMAND xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin
    -no-emul-boot -boot-load-size 4 --boot-info-table
    --efi-boot boot/limine/limine-uefi-cd.bin
    -efi-boot-part --efi-boot-image --protective-msdos-label
    "${iso_dir}" -o "${iso_file}"
    COMMAND ${limine_dir}/limine bios-install ${iso_file}
  )
  add_custom_target(iso ALL DEPENDS ${iso_file})
else ()
  target_sources(untitled_bare_metal PRIVATE
    arch/host/host_support.c
  )

  target_compile_definitions(untitled_bare_metal PRIVATE UBM_HOSTED)

  target_compile_options(untitled_bare_metal PRIVATE
    -Wno-builtin-requires-header
  )
endif ()

define_file_basename_for_sources(untitled_bare_metal)

install(TARGETS untitled_bare_metal DESTINATION bin)
install(DIRECTORY include DESTINATION include)

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
