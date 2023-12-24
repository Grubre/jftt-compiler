#pragma once

#include "ast.hpp"
#include "error.hpp"
#include "expected.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include <vector>

namespace parser {
struct Context {
    std::vector<Declaration> declarations;
    std::vector<Command> commands;
};

struct Procedure {
    std::string name;
    // TODO: Add args field
    Context context;
};

struct Program {
    std::vector<Procedure> functions;
    Context main;
};

using program_type = Program;

class Parser {
    Parser() = delete;
    Parser(Lexer &&lexer);

    auto parse_program() -> program_type;

  private:
    Lexer lexer;
};
} // namespace parser
