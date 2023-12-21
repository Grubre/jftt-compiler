add_library(Lexer STATIC lexer.cpp)

target_link_libraries(Lexer PUBLIC Common)

target_include_directories(Lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
