#include "analyzer.hpp"
#include "instruction.hpp"
#include <unordered_map>

using namespace analyzer;

void Analyzer::check_duplicate_declarations(
    var_map &already_declared,
    const IdentifierVarCollection auto &new_declarations) {
    for (const auto &var : new_declarations) {
        if (already_declared.contains(var.identifier.lexeme)) {
            const auto previous_declaration =
                already_declared[var.identifier.lexeme];
            error(std::format("Duplicate declaration of variable '{}', "
                              "first on {}:{} and then on {}:{}",
                              var.identifier.lexeme,
                              previous_declaration.token->line,
                              previous_declaration.token->column,
                              var.identifier.line, var.identifier.column),
                  var.identifier.line, var.identifier.column);
        }
        already_declared[var.identifier.lexeme] = Variable{&var.identifier};
    }
}

void Analyzer::analyze_procedure(const ast::Procedure &procedure) {
    auto variable_declarations = var_map{};
    check_duplicate_declarations(variable_declarations, procedure.args);

    analyze_context(procedure.context, variable_declarations);
}

void Analyzer::analyze_context(const ast::Context &context, var_map variables) {
    check_duplicate_declarations(variables, context.declarations);

    analyze_commands(context.commands, variables);
}

void Analyzer::analyze_commands(const std::vector<ast::Command> &commands,
                                var_map variables) {
    for (const auto &command : commands) {
        std::visit(
            overloaded{
                [&](const ast::Assignment &assignment) {
                    analyze_assignment(assignment, variables);
                },
                [&](const ast::Read &read) {
                    analyze_variable_use(read.identifier, variables);
                },
                [&](const ast::Write &write) {
                    if (std::holds_alternative<ast::Identifier>(write.value)) {
                        analyze_variable_use(
                            std::get<ast::Identifier>(write.value), variables);
                    }
                },
                [&](const ast::While &while_) {
                    // TODO: Check condition
                    analyze_commands(while_.commands, variables);
                },
                [&](const ast::Call &call) {
                    if (!procedures.contains(call.name.lexeme)) {
                        error(std::format("Call to undeclared procedure '{}'",
                                          call.signature()),
                              call.name.line, call.name.column);
                        return;
                    }

                    if (call.arity() !=
                        procedures[call.name.lexeme].procedure->arity()) {
                        error(
                            std::format(
                                "Call to procedure {} expects {} arguments, "
                                "instead found {}",
                                procedures[call.name.lexeme]
                                    .procedure->signature(),
                                procedures[call.name.lexeme].procedure->arity(),
                                call.arity()),
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

void Analyzer::analyze_assignment(const ast::Assignment &assignment,
                                  var_map &variables) {
    analyze_variable_use(assignment.identifier, variables);

    std::visit(
        overloaded{[&](const ast::BinaryExpression &expr) {
                       if (std::holds_alternative<ast::Identifier>(expr.lhs)) {
                           analyze_variable_use(
                               std::get<ast::Identifier>(expr.lhs), variables);
                       }
                       if (std::holds_alternative<ast::Identifier>(expr.rhs)) {
                           analyze_variable_use(
                               std::get<ast::Identifier>(expr.rhs), variables);
                       }
                   },
                   [&](const ast::Value &val) {
                       if (std::holds_alternative<ast::Identifier>(val)) {
                           analyze_variable_use(std::get<ast::Identifier>(val),
                                                variables);
                       }
                   }},
        assignment.expression);
}

void Analyzer::analyze_variable_use(const ast::Identifier &identifier,
                                    const var_map &variables) {
    if (!variables.contains(identifier.name.lexeme)) {
        error(std::format("Use of undeclared variable '{}'",
                          identifier.name.lexeme),
              identifier.name.line, identifier.name.column);
    }

    if (!identifier.index.has_value()) {
        return;
    }

    if (identifier.index->token_type == TokenType::Pidentifier) {
        if (!variables.contains(identifier.index->lexeme)) {
            error(std::format("Use of undeclared variable '{}'",
                              identifier.index->lexeme),
                  identifier.index->line, identifier.index->column);
        }
    }
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

void Analyzer::error(const std::string &message, unsigned line,
                     unsigned column) {
    const auto err = Error{.source = "analyzer",
                           .message = message,
                           .line = line,
                           .column = column,
                           .is_warning = false};
    errors.push_back(err);
}

void Analyzer::warn(const std::string &message, unsigned line,
                    unsigned column) {
    const auto err = Error{.source = "analyzer",
                           .message = message,
                           .line = line,
                           .column = column,
                           .is_warning = true};
    errors.push_back(err);
}
