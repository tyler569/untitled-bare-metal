cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExternalProject)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS_DEBUG "-g -O1")
set(limine_version "v12.5.2")
set(limine_dir "${CMAKE_BINARY_DIR}/limine")
set(iso_file "${CMAKE_BINARY_DIR}/tern.iso")
set(iso_dir "${CMAKE_BINARY_DIR}/isodir")
set(archive_name "${CMAKE_BINARY_DIR}/user/initrd.tar")
file(MAKE_DIRECTORY ${iso_dir}/boot/limine)

ExternalProject_Add(
  limine
  URL
    "https://github.com/limine-bootloader/limine/releases/download/${limine_version}/limine-binary.tar.xz"
  SOURCE_DIR
    "${limine_dir}"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE

  CONFIGURE_COMMAND ""
  BUILD_COMMAND
    make -C <SOURCE_DIR>
  INSTALL_COMMAND ""
)

project(tern C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(__tern__=1)

add_compile_options(-Wall -Wextra -fbuiltin)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(user)
add_subdirectory(kern)

add_custom_command(
  DEPENDS tern ${archive_name} "${CMAKE_SOURCE_DIR}/limine.conf"
  OUTPUT ${iso_file}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine.conf ${iso_dir}/boot/limine
  COMMAND ${CMAKE_COMMAND} -E copy ${limine_dir}/limine-bios.sys ${iso_dir}/boot/limine
  COMMAND ${CMAKE_COMMAND} -E copy ${limine_dir}/limine-bios-cd.bin ${iso_dir}/boot/limine
  COMMAND ${CMAKE_COMMAND} -E copy ${limine_dir}/limine-uefi-cd.bin ${iso_dir}/boot/limine
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:tern>" ${iso_dir}/boot
  COMMAND ${CMAKE_COMMAND} -E copy ${archive_name} ${iso_dir}/boot
  COMMAND xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin
  -no-emul-boot -boot-load-size 4 --boot-info-table
  --efi-boot boot/limine/limine-uefi-cd.bin
  -efi-boot-part --efi-boot-image --protective-msdos-label
  "${iso_dir}" -o "${iso_file}"
  COMMAND ${limine_dir}/limine bios-install ${iso_file}
)
add_custom_target(iso ALL DEPENDS ${iso_file})
