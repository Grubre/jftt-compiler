#pragma once
#include "ast.hpp"
#include "common.hpp"
#include <unordered_map>

class AstOptimizer {
  public:
    AstOptimizer() = delete;
    AstOptimizer(ast::Program *program) : program(program) {}

    void calculate_procedure_call_counts_helper(const std::span<const ast::Command> commands);
    void calculate_procedure_call_counts();
    auto calculate_commands_cost(const std::span<const ast::Command> commands) -> unsigned;
    void calculate_procedure_metadata();
    void inline_procedures();

    std::unordered_map<std::string, ast::Procedure *> procedures;
    std::unordered_map<std::string, unsigned> procedure_call_counts;
    std::unordered_map<std::string, unsigned> procedure_costs;
    ast::Program *program;
};
