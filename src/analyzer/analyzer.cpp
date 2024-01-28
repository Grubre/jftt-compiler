#include "analyzer.hpp"
#include "common.hpp"
#include "instruction.hpp"
#include <iostream>
#include <unordered_map>

using namespace analyzer;

void Analyzer::check_duplicate_declarations(var_map &already_declared,
                                            const IdentifierVarCollection auto &new_declarations, bool is_args) {
    for (const auto &var : new_declarations) {
        if (already_declared.contains(var.identifier.lexeme)) {
            const auto previous_declaration = already_declared[var.identifier.lexeme];
            error(std::format("Duplicate declaration of variable '{}', "
                              "first on {}:{} and then on {}:{}",
                              var.identifier.lexeme, previous_declaration.token->line,
                              previous_declaration.token->column, var.identifier.line, var.identifier.column),
                  var.identifier.line, var.identifier.column);
        }
        already_declared[var.identifier.lexeme] = Variable{.token = &var.identifier, .is_pointer = is_args};
        already_declared[var.identifier.lexeme].initialized = is_args;
    }
}

void Analyzer::check_unused_variables(const var_map &variables) {
    for (const auto &[name, variable] : variables) {
        if (!variable.used) {
            warn(std::format("Unused variable '{}'", name), variable.token->line, variable.token->column);
        }
    }
}

void Analyzer::analyze_procedure(const ast::Procedure &procedure) {
    auto variable_declarations = var_map{};
    check_duplicate_declarations(variable_declarations, procedure.args, true);

    analyze_context(procedure.context, variable_declarations);
}

void Analyzer::analyze_context(const ast::Context &context, var_map variables) {
    check_duplicate_declarations(variables, context.declarations, false);

    analyze_commands(context.commands, variables);
}

void Analyzer::analyze_commands(const std::vector<ast::Command> &commands, var_map variables) {
    for (const auto &command : commands) {
        std::visit(overloaded{[&](const ast::Assignment &assignment) { analyze_assignment(assignment, variables); },
                              [&](const ast::Read &read) {
                                  if (!variables.contains(read.identifier.name.lexeme)) {
                                      error(
                                          std::format("Read of undeclared variable '{}'", read.identifier.name.lexeme),
                                          read.identifier.name.line, read.identifier.name.column);
                                      return;
                                  }

                                  variables[read.identifier.name.lexeme].initialized = true;

                                  analyze_variable_use(read.identifier, variables);
                              },
                              [&](const ast::Write &write) {
                                  if (std::holds_alternative<ast::Identifier>(write.value)) {
                                      analyze_variable_use(std::get<ast::Identifier>(write.value), variables);
                                  }
                              },
                              [&](const ast::While &while_) {
                                  // TODO: Check condition
                                  analyze_commands(while_.commands, variables);
                              },
                              [&](const ast::Call &call) {
                                  if (!procedures.contains(call.name.lexeme)) {
                                      error(std::format("Call to undeclared procedure '{}'", call.signature()),
                                            call.name.line, call.name.column);
                                      return;
                                  }

                                  for (auto &arg : call.args) {
                                      variables[arg.lexeme].initialized = true;
                                      analyze_variable_use(arg, variables);
                                  }

                                  if (call.arity() != procedures[call.name.lexeme].procedure->arity()) {
                                      error(std::format("Call to procedure {} expects {} arguments, "
                                                        "instead found {}",
                                                        procedures[call.name.lexeme].procedure->signature(),
                                                        procedures[call.name.lexeme].procedure->arity(), call.arity()),
                                            call.name.line, call.name.column);
                                  }

                                  // TODO: Check whether corresponding arguments are arrays /
                                  // variables
                              },
                              [&](const ast::If &if_) {
                                  // TODO: Check condition
                                  analyze_commands(if_.commands, variables);
                                  if (if_.else_commands.has_value())
                                      analyze_commands(*if_.else_commands, variables);
                              },
                              [&](const ast::Repeat &repeat) {
                                  // TODO: Check condition
                                  analyze_commands(repeat.commands, variables);
                              }},
                   command);
    }
}

void Analyzer::analyze_assignment(const ast::Assignment &assignment, var_map &variables) {
    if (variables.contains(assignment.identifier.name.lexeme)) {
        variables[assignment.identifier.name.lexeme].initialized = true;
    }

    analyze_variable_use(assignment.identifier, variables);

    std::visit(overloaded{[&](const ast::BinaryExpression &expr) {
                              if (std::holds_alternative<ast::Identifier>(expr.lhs)) {
                                  analyze_variable_use(std::get<ast::Identifier>(expr.lhs), variables);
                              }
                              if (std::holds_alternative<ast::Identifier>(expr.rhs)) {
                                  analyze_variable_use(std::get<ast::Identifier>(expr.rhs), variables);
                              }
                          },
                          [&](const ast::Value &val) {
                              if (std::holds_alternative<ast::Identifier>(val)) {
                                  analyze_variable_use(std::get<ast::Identifier>(val), variables);
                              }
                          }},
               assignment.expression);
}

void Analyzer::analyze_variable_use(const Token &pidentifier, var_map &variables) {
    if (!variables.contains(pidentifier.lexeme)) {
        error(std::format("Use of undeclared variable '{}'", pidentifier.lexeme), pidentifier.line, pidentifier.column);
    }

    variables[pidentifier.lexeme].used = true;

    if (!variables[pidentifier.lexeme].initialized) {
        warn(std::format("Usage of unitialized variable '{}'", pidentifier.lexeme), pidentifier.line,
             pidentifier.column);
    }
}

void Analyzer::analyze_variable_use(const ast::Identifier &identifier, var_map &variables) {
    analyze_variable_use(identifier.name, variables);

    if (!identifier.index.has_value()) {
        return;
    }

    if (identifier.index->token_type != TokenType::Pidentifier) {
        return;
    }

    analyze_variable_use(*identifier.index, variables);
}

auto Analyzer::analyze() -> bool {
    for (const auto &procedure : program.procedures) {
        analyze_procedure(procedure);
        procedures[procedure.name.lexeme] = Procedure{&procedure};
    }

    auto variables = var_map{};
    analyze_context(program.main, variables);

    return errors.empty();
}

void Analyzer::error(const std::string &message, unsigned line, unsigned column) {
    const auto err =
        Error{.source = "analyzer", .message = message, .line = line, .column = column, .is_warning = false};
    errors.push_back(err);
}

void Analyzer::warn(const std::string &message, unsigned line, unsigned column) {
    const auto err =
        Error{.source = "analyzer", .message = message, .line = line, .column = column, .is_warning = true};
    errors.push_back(err);
}
