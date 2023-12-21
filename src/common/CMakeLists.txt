add_library(Common STATIC error.cpp)

target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
