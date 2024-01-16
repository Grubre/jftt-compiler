#include "analyzer.hpp"
#include <unordered_map>

using namespace analyzer;

void Analyzer::check_duplicates(
    std::unordered_map<std::string, Variable> &variables,
    const IdentifierVarCollection auto &identifiers) {
    for (const auto &var : identifiers) {
        if (variables.contains(var.identifier.lexeme)) {
            const auto previous_declaration = variables[var.identifier.lexeme];
            error(std::format("Duplicate declaration of variable '{}', "
                              "first on {}:{} and then on {}:{}",
                              var.identifier.lexeme,
                              previous_declaration.token->line,
                              previous_declaration.token->column,
                              var.identifier.line, var.identifier.column),
                  var.identifier.line, var.identifier.column);
        }
        variables[var.identifier.lexeme] = Variable{&var.identifier};
    }
}

void Analyzer::analyze_procedure(const ast::Procedure &procedure) {
    auto variable_declarations = std::unordered_map<std::string, Variable>{};
    check_duplicates(variable_declarations, procedure.args);
    check_duplicates(variable_declarations, procedure.context.declarations);
}

auto Analyzer::analyze() -> bool {
    for (const auto &procedure : program.procedures) {
        analyze_procedure(procedure);
    }

    auto variables = std::unordered_map<std::string, Variable>{};
    check_duplicates(variables, program.main.declarations);

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
