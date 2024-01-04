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

auto Parser::expect(TokenType type) -> std::optional<Token> {
    if (tokens.empty()) {
        errors.push_back(
            Error{.source = error_source, .message = "Unexpected end of file"});
        return std::nullopt;
    }
    auto token = tokens.front();
    // TODO: Fix the SICKENING formatting
    if (token.token_type != type) {
        errors.push_back(Error{.source = error_source,
                               .message = "Expected " + to_string(type) +
                                          " but found " + token.lexeme,
                               .line = token.line,
                               .column = token.column});
        return std::nullopt;
    }
    tokens = tokens.subspan(1);
    return token;
}

auto Parser::parse_declarations() -> std::optional<std::vector<Declaration>> {
    auto declarations = std::vector<Declaration>{};

    const auto identifier = expect(TokenType::Pidentifier);

    if (!identifier) {
        return std::nullopt;
    }

    declarations.push_back(Declaration{.identifier = *identifier});

    while (match_next(TokenType::Comma)) {
        chop();
        const auto identifier = expect(TokenType::Pidentifier);

        if (!identifier) {
            return std::nullopt;
        }

        declarations.push_back(Declaration{.identifier = *identifier});
    }

    return declarations;
}

auto Parser::parse_commands() -> std::optional<std::vector<Command>> {
    auto commands = std::vector<Command>{};

    return commands;
}

auto Parser::parse_context() -> std::optional<Context> {
    if (!expect(TokenType::Is)) {
        return std::nullopt;
    }

    const auto declarations = parse_declarations();

    if (!expect(TokenType::In)) {
        return std::nullopt;
    }

    const auto body = parse_commands();

    if (!expect(TokenType::End)) {
        return std::nullopt;
    }

    return Context{.declarations = *declarations, .commands = *body};
}

auto Parser::parse_program() -> std::optional<program_type> {
    while (match_next(TokenType::Procedure)) {
        // TODO: Parse procedure
    }

    expect(TokenType::Program);

    const auto main = parse_context();

    program.main = *main;

    return program;
}

} // namespace parser
