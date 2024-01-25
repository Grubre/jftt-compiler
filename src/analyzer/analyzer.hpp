#pragma once

#include "ast.hpp"
#include "error.hpp"
#include <unordered_map>

namespace analyzer {
// TODO: Add information about being array and array size
struct Variable {
    const Token *token;
    bool used = false;
    bool initialized = false;
    bool is_pointer;
};

struct Procedure {
    const ast::Procedure *procedure;
    bool used = false;
};

class Analyzer {
  public:
    using var_map = std::unordered_map<std::string, Variable>;
    Analyzer() = delete;
    Analyzer(ast::Program &program) : program(program) {}

    auto analyze() -> bool;
    void analyze_procedure(const ast::Procedure &procedure);
    void analyze_context(const ast::Context &context, var_map variables);
    void analyze_commands(const std::vector<ast::Command> &commands,
                          var_map already_declared);
    void analyze_assignment(const ast::Assignment &assignment,
                            var_map &variables);
    void analyze_read(const ast::Read &read, var_map &variables);
    void analyze_write(const ast::Write &write, var_map &variables);
    void analyze_while(const ast::While &while_, var_map &variables);
    void analyze_call(const ast::Call &call, var_map &variables);

    void check_unused_variables(const var_map &variables);

    void check_duplicate_declarations(
        var_map &variables, const IdentifierVarCollection auto &identifiers,
        bool is_args);

    void analyze_variable_use(const ast::Identifier &variable,
                              var_map &variables);
    void analyze_variable_use(const Token &pidentifier, var_map &variables);

    void error(const std::string &message, unsigned line, unsigned column);
    void warn(const std::string &message, unsigned line, unsigned column);

    auto get_errors() const -> const std::span<const Error> { return errors; }

  private:
    std::unordered_map<std::string, Procedure> procedures{};
    std::vector<Error> errors{};
    ast::Program &program;
};
} // namespace analyzer
