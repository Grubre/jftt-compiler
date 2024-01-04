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

    auto parse_program() -> tl::expected<program_type, Error>;

  private:
    auto chop() -> std::optional<Token>;
    auto match_next(TokenType type) -> bool;
    auto expect(TokenType type) -> tl::expected<Token, Error>;
    auto parse_main() -> tl::expected<Context, Error>;

  private:
    std::span<Token> tokens;
    Program program{};
};
} // namespace parser
