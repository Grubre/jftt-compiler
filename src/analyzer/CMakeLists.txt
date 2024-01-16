add_library(Analyzer STATIC analyzer.cpp)

target_link_libraries(Analyzer PUBLIC Common)

target_include_directories(Analyzer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
