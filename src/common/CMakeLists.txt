add_library(Common STATIC error.cpp instruction.cpp)

target_link_libraries(Common PRIVATE fmt::fmt)

target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
