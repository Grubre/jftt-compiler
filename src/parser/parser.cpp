#include "parser.hpp"

parser::Parser::Parser(Lexer &&lexer) : lexer(std::move(lexer)) {}
