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

auto Parser::peek() -> std::optional<Token> {
    if (tokens.empty())
        return std::nullopt;
    auto token = tokens.front();
    return token;
}

template <typename... TokenTypes>
auto Parser::match_next(TokenTypes... expected) -> bool {
    if (tokens.empty())
        return false;

    std::initializer_list<TokenType> expected_types{expected...};

    if (std::find(expected_types.begin(), expected_types.end(),
                  tokens.front().token_type) == expected_types.end()) {
        return false;
    }
    return true;
}

template <typename... TokenTypes>
auto Parser::expect(TokenTypes... types) -> std::optional<Token> {
    if (tokens.empty()) {
        errors.push_back(
            Error{.source = error_source, .message = "Unexpected end of file"});
        return std::nullopt;
    }
    auto token = tokens.front();
    tokens = tokens.subspan(1);

    std::initializer_list<TokenType> expected_types{types...};

    if (std::find(expected_types.begin(), expected_types.end(),
                  token.token_type) == expected_types.end()) {
        std::string expected_types_str;
        for (auto type : expected_types) {
            if (!expected_types_str.empty())
                expected_types_str += " or ";
            expected_types_str += to_string(type);
        }

        errors.push_back(Error{.source = error_source,
                               .message = "Expected " + expected_types_str +
                                          " but found " + token.lexeme,
                               .line = token.line,
                               .column = token.column});
        return std::nullopt;
    }
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

// TODO: Possibly remove the duplication between the Expression and Condition
// structures and their respective parse functions
auto Parser::parse_expression() -> std::optional<Expression> {
    const auto lhs = parse_value();

    if (!lhs) {
        return std::nullopt;
    }

    if (match_next(TokenType::Semicolon)) {
        return *lhs;
    }

    const auto op = expect(TokenType::Plus, TokenType::Minus, TokenType::Star,
                           TokenType::Slash, TokenType::Percent);

    if (!op) {
        return std::nullopt;
    }

    const auto rhs = parse_value();

    if (!rhs) {
        return std::nullopt;
    }

    return BinaryExpression{.lhs = *lhs, .op = *op, .rhs = *rhs};
}

auto Parser::parse_condition() -> std::optional<Condition> {
    const auto lhs = parse_value();

    if (!lhs) {
        return std::nullopt;
    }

    const auto op = expect(TokenType::Equals, TokenType::BangEquals,
                           TokenType::Greater, TokenType::Less,
                           TokenType::GreaterEquals, TokenType::LessEquals);

    if (!op) {
        return std::nullopt;
    }

    const auto rhs = parse_value();

    if (!rhs) {
        return std::nullopt;
    }

    return Condition{.lhs = *lhs, .op = *op, .rhs = *rhs};
}

auto Parser::parse_assignment() -> std::optional<Command> {
    const auto identifier = parse_identifier();

    if (!identifier) {
        return std::nullopt;
    }

    if (!expect(TokenType::Walrus)) {
        return std::nullopt;
    }

    const auto expression = parse_expression();

    if (!expression) {
        return std::nullopt;
    }

    return Assignment{.identifier = *identifier, .expression = *expression};
}

auto Parser::parse_read() -> std::optional<Command> {
    const auto identifier = expect(TokenType::Pidentifier);

    if (!identifier) {
        return std::nullopt;
    }

    return Read{.identifier = *identifier};
}

auto Parser::parse_write() -> std::optional<Command> {
    const auto value = parse_value();

    if (!value) {
        return std::nullopt;
    }

    return Write{.value = *value};
}

auto Parser::parse_identifier() -> std::optional<Identifier> {
    const auto identifier = expect(TokenType::Pidentifier);

    if (!identifier) {
        return std::nullopt;
    }

    if (match_next(TokenType::Lbracket)) {
        chop();
        const auto index = expect(TokenType::Num, TokenType::Pidentifier);

        if (!index) {
            return std::nullopt;
        }

        if (!expect(TokenType::Rbracket)) {
            return std::nullopt;
        }

        return Identifier{.name = *identifier, .index = *index};
    }

    return Identifier{.name = *identifier};
}

auto Parser::parse_value() -> std::optional<Value> {
    if (match_next(TokenType::Num)) {
        const auto num = expect(TokenType::Num);

        if (!num) {
            return std::nullopt;
        }

        return Num{*num};
    }

    if (match_next(TokenType::Pidentifier)) {
        const auto identifier = parse_identifier();

        if (!identifier) {
            return std::nullopt;
        }

        return *identifier;
    }

    // TODO: Handle error
    return std::nullopt;
}

auto Parser::parse_command() -> std::optional<Command> {
    const auto next = peek();

    if (!next) {
        // TODO: Handle error in a better way
        errors.push_back(
            Error{.source = error_source, .message = "Unexpected end of file"});

        return std::nullopt;
    }

    switch (next->token_type) {
    case TokenType::Read:
        return parse_read();
    case TokenType::Write:
        return parse_write();
    case TokenType::Pidentifier:
        return parse_assignment();

    default:
        errors.push_back(Error{.source = error_source,
                               .message = "Unexpected token: " + next->lexeme,
                               .line = next->line,
                               .column = next->column});
        return std::nullopt;
    }
}

auto Parser::parse_context() -> std::optional<Context> {
    if (!expect(TokenType::Is)) {
        return std::nullopt;
    }

    const auto declarations = parse_declarations();

    if (!expect(TokenType::In)) {
        return std::nullopt;
    }

    auto commands = std::vector<Command>{};

    while (!match_next(TokenType::End)) {
        const auto command = parse_command();
        if (!command) {
            return std::nullopt;
        }

        commands.push_back(*command);
    }

    if (!expect(TokenType::End)) {
        return std::nullopt;
    }

    return Context{.declarations = *declarations, .commands = commands};
}

auto Parser::parse_program() -> std::optional<program_type> {
    while (match_next(TokenType::Procedure)) {
        if (!parse_procedure()) {
            return std::nullopt;
        }
    }

    expect(TokenType::Program);

    const auto main = parse_context();

    if (!main) {
        return std::nullopt;
    }

    program.main = *main;

    return program;
}

auto Parser::parse_procedure() -> std::optional<Procedure> {
    if (!expect(TokenType::Procedure)) {
        return std::nullopt;
    }

    const auto name = expect(TokenType::Pidentifier);

    if (!name) {
        return std::nullopt;
    }

    if (!expect(TokenType::Lparen)) {
        return std::nullopt;
    }

    auto args = std::vector<Arg>{};

    while (match_next(TokenType::T, TokenType::Pidentifier)) {
        const auto next = chop();

        switch (next->token_type) {
        case TokenType::T: {
            const auto identifier = expect(TokenType::Pidentifier);

            if (!identifier) {
                return std::nullopt;
            }

            args.push_back(Arg{.identifier = *identifier, .is_array = true});
        } break;
        case TokenType::Pidentifier:
            args.push_back(Arg{.identifier = *next, .is_array = false});
            break;
        default:
            // unreachable
            break;
        }

        if (match_next(TokenType::Comma)) {
            chop();
        }
    }

    if (!expect(TokenType::Rparen)) {
        return std::nullopt;
    }

    if (!expect(TokenType::Is)) {
        return std::nullopt;
    }

    const auto declarations = parse_declarations();

    if (!expect(TokenType::In)) {
        return std::nullopt;
    }

    auto commands = std::vector<Command>{};

    while (!match_next(TokenType::End)) {
        const auto command = parse_command();
        if (!command) {
            return std::nullopt;
        }

        commands.push_back(*command);

        if (!expect(TokenType::Semicolon)) {
            return std::nullopt;
        }
    }

    if (!expect(TokenType::End)) {
        return std::nullopt;
    }

    return Procedure{.name = *name,
                     .args = args,
                     .context = Context{.declarations = *declarations,
                                        .commands = commands}};
}

} // namespace parser
