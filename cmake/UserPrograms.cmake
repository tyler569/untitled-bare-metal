
function(add_single_file_program name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE userlib)

  define_file_basename_for_sources(${name})
  set(archive_binaries ${archive_binaries} $<TARGET_FILE:${name}> PARENT_SCOPE)
endfunction()

function(add_subdirectory_program name)
  add_subdirectory(${name})
  set(archive_binaries ${archive_binaries} $<TARGET_FILE:${name}> PARENT_SCOPE)
endfunction()
