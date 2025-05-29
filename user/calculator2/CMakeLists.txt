idl_library(calculator_idl calculator2.yaml)

add_executable(calculator2 calculator.c)
target_link_libraries(calculator2 PRIVATE calculator_idl)
target_link_libraries(calculator2 PRIVATE userlib)
