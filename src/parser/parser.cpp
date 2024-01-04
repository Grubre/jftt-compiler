#include "parser.hpp"

namespace parser {

constexpr std::string error_source = "parser";

auto Parser::chop() -> std::optional<Token> {
    if (tokens.empty())
        return std::nullopt;
    auto token = tokens.front();
    tokens = tokens.subspan(1);
    return token;
}

auto Parser::match_next(TokenType type) -> bool {
    if (tokens.empty())
        return false;
    return tokens.front().token_type == type;
}

auto Parser::expect(TokenType type) -> tl::expected<Token, Error> {
    if (tokens.empty())
        return tl::unexpected(
            Error{.source = error_source, .message = "Unexpected end of file"});
    auto token = tokens.front();
    // TODO: Fix the SICKENING formatting
    if (token.token_type != type)
        return tl::unexpected(Error{.source = error_source,
                                    .message = "Expected " + to_string(type) +
                                               " but found " + token.lexeme,
                                    .line = token.line,
                                    .column = token.column});
    tokens = tokens.subspan(1);
    return token;
}

auto Parser::parse_main() -> tl::expected<Context, Error> { assert(false); }

auto Parser::parse_program() -> tl::expected<program_type, Error> {
    while (match_next(TokenType::Procedure)) {
        // TODO: Parse procedure
    }

    expect(TokenType::Program);
    expect(TokenType::Is);

    return program;
}

} // namespace parser
