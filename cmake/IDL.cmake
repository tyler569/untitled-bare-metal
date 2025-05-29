# Get full path to venv python
find_program(PYTHON_VENV_EXECUTABLE NAMES python PATHS ${CMAKE_SOURCE_DIR}/venv/bin NO_DEFAULT_PATH)

if(NOT PYTHON_VENV_EXECUTABLE)
  message(FATAL_ERROR "Python venv not found! Run 'python3 -m venv venv' first.")
endif()

function(idl_library target_name yaml_path)
  get_filename_component(base_name ${yaml_path} NAME_WE)
  set(out_dir ${CMAKE_CURRENT_BINARY_DIR}/idl/${base_name})
  set(yaml_file ${CMAKE_CURRENT_SOURCE_DIR}/${yaml_path})

  file(MAKE_DIRECTORY ${out_dir})

  add_custom_command(
    OUTPUT
      ${out_dir}/${base_name}.h
      ${out_dir}/${base_name}_client.c
      ${out_dir}/${base_name}_server.c
    COMMAND ${PYTHON_VENV_EXECUTABLE} ${PROJECT_SOURCE_DIR}/idlgen/generator.py
    ${yaml_file} ${out_dir}
    DEPENDS
      ${yaml_file}
      ${PROJECT_SOURCE_DIR}/idlgen/generator.py
      ${PROJECT_SOURCE_DIR}/idlgen/templates/header.j2
      ${PROJECT_SOURCE_DIR}/idlgen/templates/client.j2
      ${PROJECT_SOURCE_DIR}/idlgen/templates/server.j2
    COMMENT "Generating IDL code for ${base_name}"
    VERBATIM
  )

  add_custom_target(${target_name}_generate
    DEPENDS
      ${out_dir}/${base_name}.h
      ${out_dir}/${base_name}_client.c
      ${out_dir}/${base_name}_server.c
  )

  add_library(${target_name} STATIC
    ${out_dir}/${base_name}_server.c
    ${out_dir}/${base_name}_client.c
  )

  add_dependencies(${target_name} ${target_name}_generate)
  set_target_properties(${target_name} PROPERTIES PUBLIC_HEADER ${out_dir}/${base_name}.h)
  target_include_directories(${target_name} PUBLIC ${out_dir})
endfunction()

