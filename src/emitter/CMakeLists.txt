add_library(Emitter STATIC emitter.cpp)

target_link_libraries(Emitter PUBLIC Common Parser)

target_include_directories(Emitter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
