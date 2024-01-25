#pragma once

#include "ast.hpp"
#include "error.hpp"
#include "expected.hpp"
#include "token.hpp"
#include <span>
#include <vector>

namespace parser {

using program_type = ast::Program;

class Parser {
  public:
    Parser() = delete;
    Parser(const std::span<Token> &tokens) : tokens(tokens){};

    auto get_errors() const -> const std::vector<Error> & { return errors; }

    auto parse_program() -> std::optional<program_type>;
    auto parse_procedure() -> std::optional<ast::Procedure>;
    auto chop() -> std::optional<Token>;
    auto match_and_chop(TokenType type) -> std::optional<Token>;
    auto peek(uint64_t offset = 0) -> std::optional<Token>;
    template <typename... TokenTypes> auto match_next(TokenTypes... expected) -> bool;
    template <typename... TokenTypes> auto expect(TokenTypes... types) -> std::optional<Token>;
    auto parse_declarations() -> std::optional<std::vector<ast::Declaration>>;
    auto parse_command() -> std::optional<ast::Command>;
    auto parse_context() -> std::optional<ast::Context>;
    auto parse_read() -> std::optional<ast::Command>;
    auto parse_write() -> std::optional<ast::Command>;
    auto parse_assignment() -> std::optional<ast::Command>;
    auto parse_expression() -> std::optional<ast::Expression>;
    auto parse_condition() -> std::optional<ast::Condition>;
    auto parse_identifier() -> std::optional<ast::Identifier>;
    auto parse_value() -> std::optional<ast::Value>;
    auto parse_while() -> std::optional<ast::Command>;
    auto parse_call() -> std::optional<ast::Command>;
    auto parse_if() -> std::optional<ast::Command>;
    auto parse_repeat() -> std::optional<ast::Command>;

  private:
    std::span<Token> tokens;

    std::vector<Error> errors{};
    ast::Program program{};
};
} // namespace parser
