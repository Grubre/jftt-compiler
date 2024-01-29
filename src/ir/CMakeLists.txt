add_library(Ir STATIC low_level_ir_builder.cpp high_level_ir.cpp low_level_ir.cpp
                      cfg_builder.cpp)

target_link_libraries(Ir PUBLIC Common)

target_include_directories(Ir PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
