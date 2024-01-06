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

struct Arg {
    Token identifier;
    bool is_array;
};

struct Procedure {
    Token name;
    std::vector<Arg> args{};
    Context context;
};

struct Program {
    std::vector<Procedure> procedures{};
    Context main;
};

using program_type = Program;

class Parser {
  public:
    Parser() = delete;
    Parser(const std::span<Token> &tokens) : tokens(tokens){};

    auto get_errors() const -> const std::vector<Error> & { return errors; }

    auto parse_program() -> std::optional<program_type>;
    auto parse_procedure() -> std::optional<Procedure>;
    auto chop() -> std::optional<Token>;
    auto match_and_chop(TokenType type) -> std::optional<Token>;
    auto peek(uint64_t offset = 0) -> std::optional<Token>;
    template <typename... TokenTypes>
    auto match_next(TokenTypes... expected) -> bool;
    template <typename... TokenTypes>
    auto expect(TokenTypes... types) -> std::optional<Token>;
    auto parse_declarations() -> std::optional<std::vector<Declaration>>;
    auto parse_command() -> std::optional<Command>;
    auto parse_context() -> std::optional<Context>;
    auto parse_read() -> std::optional<Command>;
    auto parse_write() -> std::optional<Command>;
    auto parse_assignment() -> std::optional<Command>;
    auto parse_expression() -> std::optional<Expression>;
    auto parse_condition() -> std::optional<Condition>;
    auto parse_identifier() -> std::optional<Identifier>;
    auto parse_value() -> std::optional<Value>;
    auto parse_while() -> std::optional<Command>;
    auto parse_call() -> std::optional<Command>;
    auto parse_if() -> std::optional<Command>;
    auto parse_repeat() -> std::optional<Command>;

  private:
    std::span<Token> tokens;

    std::vector<Error> errors{};
    Program program{};
};
} // namespace parser
