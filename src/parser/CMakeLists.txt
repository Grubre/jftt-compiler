add_library(Parser STATIC parser.cpp)

target_link_libraries(Parser PUBLIC Common)

target_include_directories(Parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
