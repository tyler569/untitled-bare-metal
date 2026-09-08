add_library(userlib STATIC
  cptr_alloc.c
  exec.c
  lib.c
  pci.c
  num.c
  print.c
  sort.c
  string.c
  tar.c
)

target_include_directories(userlib PUBLIC .)

if ("${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "Generic x86_64")
  target_sources(userlib PRIVATE
    arch/x86_64/crt0.c
  )
else ()
  message(FATAL_ERROR "Unsupported target: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# C++ entry ABI: extern "C" int tern_main(word_t, ...), up to six words.
add_library(userlib_cxx INTERFACE)
target_link_libraries(userlib_cxx INTERFACE userlib)
target_sources(userlib_cxx INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/cxx_start.S
)
