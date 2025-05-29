idl_library(calculator_idl calculator.yaml)

add_executable(calculator calculator.c)
target_link_libraries(calculator PRIVATE calculator_idl)
target_link_libraries(calculator PRIVATE userlib)
