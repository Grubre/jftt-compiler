add_subdirectory(common)
add_subdirectory(lexer)
add_subdirectory(parser)

add_executable(compiler compiler.cpp)
target_link_libraries(compiler Common Lexer Parser)
