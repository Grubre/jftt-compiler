#pragma once

#include "error.hpp"
#include "expected.hpp"
#include "lexer.hpp"
#include "token.hpp"

using program_type = tl::expected<void, void>;

class Parser {
    Parser() = delete;
    Parser(Lexer&& lexer);

    auto parse_program() -> program_type;

  private:
    Lexer lexer;
};
