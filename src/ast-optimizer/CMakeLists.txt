add_library(AstOptimizer STATIC ast-optimizer.cpp)

target_link_libraries(AstOptimizer PUBLIC Common)

target_include_directories(AstOptimizer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
