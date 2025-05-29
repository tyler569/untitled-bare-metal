# Define the library if it hasn't already been defined
if (NOT TARGET userlib)
  add_library(userlib STATIC
    user/cptr_alloc.c
    user/exec.c
    user/lib.c
    user/pci.c
    lib/num.c
    lib/print.c
    lib/sort.c
    lib/string.c
    lib/tar.c
  )
endif ()

function(add_single_file_program name)
  add_executable(${name} user/${name}.c)
  target_link_libraries(${name} PRIVATE userlib)

  define_file_basename_for_sources(${name})
  set(archive_binaries ${archive_binaries} $<TARGET_FILE:${name}> PARENT_SCOPE)
endfunction()

function(add_subdirectory_program name)
  include_directories(user)
  add_subdirectory(user/${name})
  set(archive_binaries ${archive_binaries} $<TARGET_FILE:${name}> PARENT_SCOPE)
endfunction()
