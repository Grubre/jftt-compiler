add_library(TestVMcln STATIC mw-cln.cc)

target_link_libraries(TestVMcln PUBLIC TestVM Common Emitter cln)

target_include_directories(TestVMcln PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
