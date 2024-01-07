set(TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples2023")

function(create_test test_name source_file libs)
  add_executable(${test_name} ${source_file})
  target_link_libraries(${test_name} Doctest Lexer ${libs})
  target_compile_definitions(${test_name} PRIVATE TESTS_DIR="${TESTS_DIR}")
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

create_test(lexer_test lexer_test.cpp Lexer)
create_test(parser_test parser_test.cpp Parser)
