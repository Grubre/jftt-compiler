#pragma once

#include "ast.hpp"
#include "error.hpp"
#include "expected.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include <span>
#include <vector>

namespace parser {
struct Context {
    std::vector<Declaration> declarations{};
    std::vector<Command> commands{};
};

struct Procedure {
    std::string name{};
    // TODO: Add args field
    Context context{};
};

struct Program {
    std::vector<Procedure> functions{};
    Context main{};
};

using program_type = Program;

class Parser {
  public:
    Parser() = delete;
    Parser(const std::span<Token> &tokens) : tokens(tokens){};

    auto parse_program() -> std::optional<program_type>;

  private:
    auto chop() -> std::optional<Token>;
    auto match_next(TokenType type) -> bool;
    auto expect(TokenType type) -> std::optional<Token>;
    auto parse_context() -> std::optional<Context>;
    auto parse_declarations() -> std::optional<std::vector<Declaration>>;
    auto parse_commands() -> std::optional<std::vector<Command>>;

  private:
    std::span<Token> tokens;

    std::vector<Error> errors{};
    Program program{};
};
} // namespace parser
