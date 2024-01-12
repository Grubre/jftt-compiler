#include "parser.hpp"
#include <format>
#include <iostream>
#include <optional>

namespace parser {

constexpr std::string error_source = "parser";

auto Parser::chop() -> std::optional<Token> {
    if (tokens.empty())
        return std::nullopt;
    auto token = tokens.front();
    tokens = tokens.subspan(1);
    return token;
}

auto Parser::match_and_chop(TokenType type) -> std::optional<Token> {
    if (tokens.empty())
        return std::nullopt;
    auto token = tokens.front();
    if (token.token_type != type)
        return std::nullopt;
    tokens = tokens.subspan(1);
    return token;
}

auto Parser::peek(uint64_t offset) -> std::optional<Token> {
    if (tokens.size() <= offset)
        return std::nullopt;
    return tokens[offset];
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

    while (match_next(TokenType::Pidentifier)) {
        std::optional<Token> array_size = std::nullopt;
        const auto identifier = chop();

        if (match_next(TokenType::Lbracket)) {
            chop();

            array_size = expect(TokenType::Num);

            if (!array_size) {
                return std::nullopt;
            }

            if (!expect(TokenType::Rbracket)) {
                return std::nullopt;
            }
        }

        declarations.push_back(
            Declaration{.identifier = *identifier, .array_size = array_size});

        if (match_next(TokenType::Comma)) {
            chop();
        }
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

    if (!expect(TokenType::Semicolon)) {
        return std::nullopt;
    }

    return Assignment{.identifier = *identifier, .expression = *expression};
}

auto Parser::parse_read() -> std::optional<Command> {
    if (!expect(TokenType::Read)) {
        return std::nullopt;
    }

    const auto identifier = parse_identifier();

    if (!identifier) {
        return std::nullopt;
    }

    if (!expect(TokenType::Semicolon)) {
        return std::nullopt;
    }

    return Read{.identifier = *identifier};
}

auto Parser::parse_write() -> std::optional<Command> {
    if (!expect(TokenType::Write)) {
        return std::nullopt;
    }

    const auto value = parse_value();

    if (!value) {
        return std::nullopt;
    }

    if (!expect(TokenType::Semicolon)) {
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

    errors.push_back(Error{
        .source = error_source,
        .message =
            std::format("Expected a number or identifier, instead found '{}'",
                        peek()->lexeme),
        .line = peek()->line,
        .column = peek()->column});

    return std::nullopt;
}

auto Parser::parse_command() -> std::optional<Command> {
    const auto next = peek();

    if (!next) {
        errors.push_back(
            Error{.source = error_source, .message = "Unexpected end of file"});

        return std::nullopt;
    }

    switch (next->token_type) {
    case TokenType::Read:
        return parse_read();
    case TokenType::Write:
        return parse_write();
    case TokenType::If:
        return parse_if();
    case TokenType::Repeat:
        return parse_repeat();
    case TokenType::Pidentifier: {
        const auto token_after_identifier = peek(1);

        if (token_after_identifier.has_value() &&
            token_after_identifier->token_type == TokenType::Lparen)
            return parse_call();

        const auto assignment = parse_assignment();

        if (assignment)
            return *assignment;

        errors.push_back(
            Error{.source = error_source,
                  .message = "Expected ':=' or '(<args>)' after identifier " +
                             next->lexeme,
                  .line = next->line,
                  .column = next->column});

        return std::nullopt;
    }
    case TokenType::While:
        return parse_while();
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
        const auto procedure = parse_procedure();
        if (!procedure) {
            return std::nullopt;
        }

        program.procedures.push_back(*procedure);
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
    }

    if (!expect(TokenType::End)) {
        return std::nullopt;
    }

    return Procedure{.name = *name,
                     .args = args,
                     .context = Context{.declarations = *declarations,
                                        .commands = commands}};
}

auto Parser::parse_while() -> std::optional<Command> {
    if (!expect(TokenType::While)) {
        return std::nullopt;
    }

    const auto condition = parse_condition();

    if (!condition) {
        return std::nullopt;
    }

    if (!expect(TokenType::Do)) {
        return std::nullopt;
    }

    auto commands = std::vector<Command>{};

    while (!match_next(TokenType::EndWhile)) {
        const auto command = parse_command();
        if (!command) {
            return std::nullopt;
        }

        commands.push_back(*command);
    }

    if (!expect(TokenType::EndWhile)) {
        return std::nullopt;
    }

    return While{.condition = *condition, .commands = commands};
}

auto Parser::parse_call() -> std::optional<Command> {
    const auto name = expect(TokenType::Pidentifier);

    if (!name) {
        return std::nullopt;
    }

    if (!expect(TokenType::Lparen)) {
        return std::nullopt;
    }

    auto args = std::vector<Token>{};

    while (match_next(TokenType::Pidentifier)) {
        const auto next = chop();

        args.push_back(*next);

        if (match_next(TokenType::Comma)) {
            chop();
        }
    }

    if (!expect(TokenType::Rparen)) {
        return std::nullopt;
    }

    if (!expect(TokenType::Semicolon)) {
        return std::nullopt;
    }

    return Call{.name = *name, .args = args};
}

auto Parser::parse_if() -> std::optional<Command> {
    if (!expect(TokenType::If)) {
        return std::nullopt;
    }

    const auto condition = parse_condition();

    if (!condition) {
        return std::nullopt;
    }

    if (!expect(TokenType::Then)) {
        return std::nullopt;
    }

    auto commands = std::vector<Command>{};

    while (!match_next(TokenType::EndIf, TokenType::Else)) {
        const auto command = parse_command();
        if (!command) {
            return std::nullopt;
        }

        commands.push_back(*command);
    }

    std::optional<std::vector<Command>> else_commands = std::nullopt;

    if (match_next(TokenType::Else)) {
        chop();

        else_commands = std::vector<Command>{};

        while (!match_next(TokenType::EndIf)) {
            const auto command = parse_command();
            if (!command) {
                return std::nullopt;
            }

            else_commands->push_back(*command);
        }
    }

    if (!expect(TokenType::EndIf)) {
        return std::nullopt;
    }

    return If{.condition = *condition,
              .commands = commands,
              .else_commands = else_commands};
}

auto Parser::parse_repeat() -> std::optional<Command> {
    if (!expect(TokenType::Repeat)) {
        return std::nullopt;
    }

    auto commands = std::vector<Command>{};

    while (!match_next(TokenType::Until)) {
        const auto command = parse_command();
        if (!command) {
            return std::nullopt;
        }

        commands.push_back(*command);
    }

    if (!expect(TokenType::Until)) {
        return std::nullopt;
    }

    const auto condition = parse_condition();

    if (!condition) {
        return std::nullopt;
    }

    if (!expect(TokenType::Semicolon)) {
        return std::nullopt;
    }

    return Repeat{.commands = commands, .condition = *condition};
}

} // namespace parser
