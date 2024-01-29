#include "ast-optimizer.hpp"
#include <iostream>

void substitute_identifier(const std::unordered_map<std::string, std::string> &mapped_args,
                           ast::Identifier &identifier) {
    identifier.name.lexeme = mapped_args.at(identifier.name.lexeme);

    if (identifier.index) {
        identifier.index->lexeme = mapped_args.at(identifier.index->lexeme);
    }
}

void substitute_value(const std::unordered_map<std::string, std::string> &mapped_args, ast::Value &value) {
    if (std::holds_alternative<ast::Identifier>(value)) {
        substitute_identifier(mapped_args, std::get<ast::Identifier>(value));
    }
}

void substitute_condition(const std::unordered_map<std::string, std::string> &mapped_args, ast::Condition &condition) {
    substitute_value(mapped_args, condition.lhs);
    substitute_value(mapped_args, condition.rhs);
}

void change_inlined_occurences(const std::unordered_map<std::string, std::string> &mapped_args,
                               std::span<ast::Command> commands) {
    for (auto &command : commands) {
        std::visit(overloaded{[&](ast::Read &read) { substitute_identifier(mapped_args, read.identifier); },
                              [&](ast::Write &write) { substitute_value(mapped_args, write.value); },
                              [&](ast::If &if_statement) {
                                  substitute_condition(mapped_args, if_statement.condition);
                                  change_inlined_occurences(mapped_args, if_statement.commands);
                                  if (if_statement.else_commands) {
                                      change_inlined_occurences(mapped_args, *if_statement.else_commands);
                                  }
                              },
                              [&](ast::Repeat &repeat) {
                                  substitute_condition(mapped_args, repeat.condition);
                                  change_inlined_occurences(mapped_args, repeat.commands);
                              },
                              [&](ast::Assignment &assignment) {
                                  substitute_identifier(mapped_args, assignment.identifier);
                                  if (std::holds_alternative<ast::Value>(assignment.expression)) {
                                      substitute_value(mapped_args, std::get<ast::Value>(assignment.expression));
                                  } else {
                                      auto &binary_expression = std::get<ast::BinaryExpression>(assignment.expression);
                                      substitute_value(mapped_args, binary_expression.lhs);
                                      substitute_value(mapped_args, binary_expression.rhs);
                                  }
                              },
                              [&](ast::While &while_statement) {
                                  substitute_condition(mapped_args, while_statement.condition);
                                  change_inlined_occurences(mapped_args, while_statement.commands);
                              },
                              [&](ast::Call &call) {
                                  for (auto &arg : call.args) {
                                      arg.lexeme = mapped_args.at(arg.lexeme);
                                  }
                              },
                              [&](ast::InlinedProcedure &procedure) {
                                  change_inlined_occurences(mapped_args, procedure.commands);
                              }},
                   command);
    }
}

void AstOptimizer::inline_procedures() {
    for (auto &procedure : program->procedures) {
        procedures[procedure.name.lexeme] = &procedure;
        for (auto &command : procedure.context.commands) {

            std::visit(overloaded{[&](const ast::Call &call) {
                                      bool should_be_inlined = false;
                                      // TODO: Find a better way to determine if procedure should be inlined
                                      if (!should_be_inlined) {
                                          return;
                                      }

                                      const auto inlined_procedure = procedures[call.name.lexeme];
                                      auto procedure_commands_copy = procedures[call.name.lexeme]->context.commands;

                                      auto mapped_args = std::unordered_map<std::string, std::string>{};
                                      for (auto i = 0u; i < inlined_procedure->args.size(); ++i) {
                                          mapped_args[inlined_procedure->args[i].identifier.lexeme] =
                                              call.args[i].lexeme;
                                      }

                                      for (auto i = 0u; i < inlined_procedure->context.declarations.size(); ++i) {
                                          const auto name =
                                              inlined_procedure->context.declarations[i].identifier.lexeme;
                                          auto declaration = inlined_procedure->context.declarations[i];
                                          mapped_args[name] = name + "@" + call.signature();
                                          declaration.identifier.lexeme = mapped_args[name];
                                          procedure.context.declarations.push_back(declaration);
                                      }

                                      change_inlined_occurences(mapped_args, procedure_commands_copy);

                                      command = ast::InlinedProcedure{procedure_commands_copy};
                                  },
                                  [&](const auto &) {}},
                       command);
        }
    }
}

void AstOptimizer::calculate_procedure_call_counts_helper(const std::span<const ast::Command> commands) {
    for (const auto &command : commands) {
        std::visit(
            overloaded{[&](const ast::If &if_statement) { calculate_commands_cost(if_statement.commands); },
                       [&](const ast::Repeat &repeat) { calculate_commands_cost(repeat.commands); },
                       [&](const ast::While &while_statement) { calculate_commands_cost(while_statement.commands); },
                       [&](const ast::Call &call) { procedure_call_counts[call.name.lexeme] += 1; },
                       [&](const auto &) {}},
            command);
    }
}

void AstOptimizer::calculate_procedure_call_counts() {
    for (const auto &procedure : program->procedures) {
        calculate_procedure_call_counts_helper(procedure.context.commands);
    }

    calculate_procedure_call_counts_helper(program->main.commands);
}

auto AstOptimizer::calculate_commands_cost(const std::span<const ast::Command> commands) -> unsigned {
    auto len = 0u;
    for (const auto &command : commands) {
        std::visit(overloaded{[&](const ast::Read &read) { len += 1; }, [&](const ast::Write &write) {},
                              [&](const ast::If &if_statement) {}, [&](const ast::Repeat &repeat) {},
                              [&](const ast::Assignment &assignment) {}, [&](const ast::While &while_statement) {},
                              [&](const ast::Call &call) {}, [&](const ast::InlinedProcedure &procedure) {}},
                   command);
    }
    return len;
}

void AstOptimizer::calculate_procedure_metadata() {
    for (const auto &procedure : program->procedures) {
        auto cost = 0u;
        cost += calculate_commands_cost(procedure.context.commands);
        procedure_costs[procedure.name.lexeme] = cost;
    }
}
