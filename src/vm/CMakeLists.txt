add_library(TestVM STATIC mw.cc)

target_link_libraries(TestVM PUBLIC Common)

target_include_directories(TestVM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
