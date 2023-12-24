#include "parser.hpp"

 Parser::Parser(Lexer&& lexer) : lexer(std::move(lexer)) {}
