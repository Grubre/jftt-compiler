add_subdirectory(common)
add_subdirectory(ir)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(analyzer)
add_subdirectory(ast-optimizer)
add_subdirectory(emitter)
add_subdirectory(vm)
add_subdirectory(vm-cln)

add_executable(compiler compiler.cpp)
target_link_libraries(
  compiler
  Common
  Lexer
  Parser
  Emitter
  Analyzer
  TestVM
  TestVMcln
  Ir
  AstOptimizer)
