cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalProject)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS_DEBUG "-g -O1")
set(limine_dir "${CMAKE_BINARY_DIR}/limine")
set(iso_file "${CMAKE_BINARY_DIR}/untitled_bare_metal.iso")
set(iso_dir "${CMAKE_BINARY_DIR}/isodir")
set(archive_name "${CMAKE_BINARY_DIR}/user/initrd.tar")
file(MAKE_DIRECTORY ${iso_dir}/boot/limine)

# We do this as a manual git download as a workaround for the fact that
# ExternalProject_Add doesn't support shallow clones, or more accurately
# lies about its support for shallow clones.
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/17770
if (NOT EXISTS ${limine_dir})
  message(STATUS "Cloning limine bootloader...")
  execute_process(
    COMMAND git clone --depth 1 --branch v10.x-binary
            https://github.com/limine-bootloader/limine.git ${limine_dir}
    RESULT_VARIABLE git_result
  )
  if (NOT git_result EQUAL 0)
    message(FATAL_ERROR "Failed to clone limine")
  endif()
endif()

# Build limine binary
ExternalProject_Add(limine_binary
  SOURCE_DIR "${limine_dir}"
  CONFIGURE_COMMAND ""
  BUILD_IN_SOURCE 1
  BUILD_COMMAND make
  INSTALL_COMMAND ""
  UPDATE_COMMAND ""
)

project(untitled_bare_metal C ASM)

add_compile_definitions(__shrike__=1)

add_compile_options(-Wall -Wextra -fbuiltin)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/limine)

add_subdirectory(user)
add_subdirectory(kern)

add_custom_command(
  DEPENDS limine_binary untitled_bare_metal ${archive_name} "${CMAKE_SOURCE_DIR}/limine.conf"
  OUTPUT ${iso_file}
  COMMAND cp ${CMAKE_SOURCE_DIR}/limine.conf ${iso_dir}/boot/limine
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
