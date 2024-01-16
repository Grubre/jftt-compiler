#include "analyzer.hpp"
#include <unordered_map>

void Analyzer::analyze_procedure(const ast::Procedure &procedure) {
    auto variable_declarations =
        std::unordered_map<std::string, const Token *>{};

    const auto check_duplicates = [&](const auto &collection) {
        for (const auto &var : collection) {
            if (variable_declarations.contains(var.identifier.lexeme)) {
                const auto previous_declaration =
                    variable_declarations[var.identifier.lexeme];
                error(std::format("Duplicate declaration of variable '{}', "
                                  "first on {}:{} and then on {}:{}",
                                  var.identifier.lexeme,
                                  previous_declaration->line,
                                  previous_declaration->column,
                                  var.identifier.line, var.identifier.column),
                      var.identifier.line, var.identifier.column);
            }
            variable_declarations[var.identifier.lexeme] = &var.identifier;
        }
    };

    check_duplicates(procedure.args);
    check_duplicates(procedure.context.declarations);
}

auto Analyzer::analyze() -> bool {
    for (const auto &procedure : program.procedures) {
        analyze_procedure(procedure);
    }

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
