#pragma once

#include "ast.hpp"
#include "error.hpp"
class Analyzer {
  public:
    Analyzer() = delete;
    Analyzer(ast::Program &program) : program(program) {}

    auto analyze() -> bool;
    void analyze_procedure(const ast::Procedure &procedure);

    void error(const std::string &message, unsigned line, unsigned column);
    void warn(const std::string &message, unsigned line, unsigned column);

    auto get_errors() const -> const std::span<const Error> { return errors; }

  private:
    std::vector<Error> errors{};
    ast::Program &program;
};
