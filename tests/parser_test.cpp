#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "parser.hpp"
#include "tests_shared.hpp"
#include "token.hpp"
#include <variant>

auto make_pidentifier(std::string name) -> Token {
    return Token{.token_type = TokenType::Pidentifier,
                 .lexeme = std::move(name),
                 .line = 1,
                 .column = 1};
}

auto make_comma() -> Token {
    return Token{
        .token_type = TokenType::Comma, .lexeme = ",", .line = 1, .column = 1};
}

auto make_num(std::string value) -> Token {
    return Token{.token_type = TokenType::Num,
                 .lexeme = std::move(value),
                 .line = 1,
                 .column = 1};
}

auto make_lbracket() -> Token {
    return Token{.token_type = TokenType::Lbracket,
                 .lexeme = "[",
                 .line = 1,
                 .column = 1};
}

auto make_rbracket() -> Token {
    return Token{.token_type = TokenType::Rbracket,
                 .lexeme = "]",
                 .line = 1,
                 .column = 1};
}

auto make_operator(char op) -> Token {
    switch (op) {
    case '+':
        return Token{.token_type = TokenType::Plus,
                     .lexeme = std::string{op},
                     .line = 1,
                     .column = 1};
    case '-':
        return Token{.token_type = TokenType::Minus,
                     .lexeme = std::string{op},
                     .line = 1,
                     .column = 1};
    case '*':
        return Token{.token_type = TokenType::Star,
                     .lexeme = std::string{op},
                     .line = 1,
                     .column = 1};
    case '/':
        return Token{.token_type = TokenType::Slash,
                     .lexeme = std::string{op},
                     .line = 1,
                     .column = 1};
    case '%':
        return Token{.token_type = TokenType::Percent,
                     .lexeme = std::string{op},
                     .line = 1,
                     .column = 1};
    }
    assert(false);
}

TEST_CASE("Parser - constructor") {
    SUBCASE("Empty token array") {
        auto tokens = std::vector<Token>{};
        auto parser = parser::Parser(tokens);
    }

    SUBCASE("Non-empty token array") {
        auto tokens = std::vector<Token>{make_pidentifier("n")};
        auto parser = parser::Parser(tokens);
    }
}

TEST_CASE("Parser - chop") {
    auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(),
                                     make_pidentifier("m")};
    auto parser = parser::Parser(tokens);

    const auto n = parser.chop();
    CHECK(n.has_value());
    CHECK(n->token_type == TokenType::Pidentifier);
    CHECK(n->lexeme == "n");

    const auto comma = parser.chop();
    CHECK(comma.has_value());
    CHECK(comma->token_type == TokenType::Comma);
    CHECK(comma->lexeme == ",");

    const auto m = parser.chop();
    CHECK(m.has_value());
    CHECK(m->token_type == TokenType::Pidentifier);
    CHECK(m->lexeme == "m");

    const auto end = parser.chop();
    CHECK(!end.has_value());
}

TEST_CASE("Parser - match_next") {
    auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(),
                                     make_pidentifier("m")};
    auto parser = parser::Parser(tokens);

    CHECK(parser.match_next(TokenType::Pidentifier));
    parser.chop();
    CHECK(parser.match_next(TokenType::Comma));
    parser.chop();
    CHECK(parser.match_next(TokenType::Pidentifier));
    parser.chop();
    CHECK(!parser.match_next(TokenType::Pidentifier));
}

TEST_CASE("Parser - expect") {
    auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(),
                                     make_pidentifier("m")};
    auto parser = parser::Parser(tokens);

    const auto n = parser.expect(TokenType::Pidentifier);
    CHECK(n.has_value());
    CHECK(n->token_type == TokenType::Pidentifier);
    CHECK(n->lexeme == "n");

    const auto comma = parser.expect(TokenType::Comma);
    CHECK(comma.has_value());
    CHECK(comma->token_type == TokenType::Comma);
    CHECK(comma->lexeme == ",");

    const auto m = parser.expect(TokenType::Pidentifier);
    CHECK(m.has_value());
    CHECK(m->token_type == TokenType::Pidentifier);
    CHECK(m->lexeme == "m");

    const auto end = parser.expect(TokenType::Pidentifier);
    CHECK(!end.has_value());
    CHECK(parser.get_errors().size() == 1);
}

TEST_CASE("Parser - parse declarations") {
    SUBCASE("Single declaration") {
        auto tokens = std::vector<Token>{make_pidentifier("n")};
        auto parser = parser::Parser(tokens);

        const auto declarations = parser.parse_declarations();
        CHECK(declarations.has_value());
        CHECK(declarations->size() == 1);
        CHECK(declarations->front().identifier.lexeme == "n");
    }

    SUBCASE("Multiple declarations") {
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(),
                                         make_pidentifier("m")};
        auto parser = parser::Parser(tokens);

        const auto declarations = parser.parse_declarations();
        CHECK(declarations.has_value());
        CHECK(declarations->size() == 2);
        CHECK(declarations->front().identifier.lexeme == "n");
        CHECK(declarations->back().identifier.lexeme == "m");
    }

    SUBCASE("Missing identifier") {
        auto tokens = std::vector<Token>{make_comma()};
        auto parser = parser::Parser(tokens);

        const auto declarations = parser.parse_declarations();
        CHECK(!declarations.has_value());
        CHECK(parser.get_errors().size() == 1);
    }
}

TEST_CASE("Parser - parse identifier") {
    SUBCASE("No index") {
        auto tokens = std::vector<Token>{make_pidentifier("n")};
        auto parser = parser::Parser(tokens);

        const auto identifier = parser.parse_identifier();
        CHECK(identifier.has_value());
        CHECK(identifier->name.lexeme == "n");
        CHECK(!identifier->index.has_value());
    }

    SUBCASE("Num index") {
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_lbracket(),
                                         make_num("0"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto identifier = parser.parse_identifier();
        CHECK(identifier.has_value());
        CHECK(identifier->name.lexeme == "n");
        CHECK(identifier->index.has_value());
        CHECK(identifier->index->lexeme == "0");
    }

    SUBCASE("Pidentifier index") {
        auto tokens =
            std::vector<Token>{make_pidentifier("n"), make_lbracket(),
                               make_pidentifier("m"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto identifier = parser.parse_identifier();
        CHECK(identifier.has_value());
        CHECK(identifier->name.lexeme == "n");
        CHECK(identifier->index.has_value());
        CHECK(identifier->index->lexeme == "m");
    }
}

TEST_CASE("Parser - parse value") {
    SUBCASE("Num") {
        auto tokens = std::vector<Token>{make_num("0")};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<parser::Num>(*value));
        CHECK(std::get<parser::Num>(*value).lexeme == "0");
    }

    SUBCASE("Identifier") {
        auto tokens = std::vector<Token>{make_pidentifier("n")};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<parser::Identifier>(*value));
        CHECK(std::get<parser::Identifier>(*value).name.lexeme == "n");
        CHECK(!std::get<parser::Identifier>(*value).index.has_value());
    }

    SUBCASE("Identifier indexed by identifier") {
        auto tokens =
            std::vector<Token>{make_pidentifier("n"), make_lbracket(),
                               make_pidentifier("m"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<parser::Identifier>(*value));
        CHECK(std::get<parser::Identifier>(*value).name.lexeme == "n");
        CHECK(std::get<parser::Identifier>(*value).index.has_value());
        CHECK(std::get<parser::Identifier>(*value).index->lexeme == "m");
    }

    SUBCASE("Identifier indexed by num") {
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_lbracket(),
                                         make_num("0"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<parser::Identifier>(*value));
        CHECK(std::get<parser::Identifier>(*value).name.lexeme == "n");
        CHECK(std::get<parser::Identifier>(*value).index.has_value());
        CHECK(std::get<parser::Identifier>(*value).index->lexeme == "0");
    }
}

TEST_CASE("Parser - parse expression") {
    SUBCASE("Num + Num") {
        auto tokens = std::vector<Token>{make_num("0"), make_operator('+'),
                                         make_num("1")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        CHECK(expression.has_value());
        CHECK(std::holds_alternative<parser::Num>(expression->lhs));
        CHECK(std::holds_alternative<parser::Num>(expression->rhs));
        CHECK(expression->op.token_type == TokenType::Plus);
    }

    SUBCASE("Num + Identifier") {
        auto tokens = std::vector<Token>{make_num("0"), make_operator('+'),
                                         make_pidentifier("n")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        CHECK(expression.has_value());
        CHECK(std::holds_alternative<parser::Num>(expression->lhs));
        CHECK(std::holds_alternative<parser::Identifier>(expression->rhs));
        CHECK(expression->op.token_type == TokenType::Plus);
    }

    SUBCASE("Identifier + Num") {
        auto tokens = std::vector<Token>{make_pidentifier("n"),
                                         make_operator('+'), make_num("1")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        CHECK(expression.has_value());
        CHECK(std::holds_alternative<parser::Identifier>(expression->lhs));
        CHECK(std::holds_alternative<parser::Num>(expression->rhs));
        CHECK(expression->op.token_type == TokenType::Plus);
    }

    SUBCASE("Identifier + Identifier") {
        auto tokens = std::vector<Token>{
            make_pidentifier("n"), make_operator('+'), make_pidentifier("m")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        CHECK(expression.has_value());
        CHECK(std::holds_alternative<parser::Identifier>(expression->lhs));
        CHECK(std::holds_alternative<parser::Identifier>(expression->rhs));
        CHECK(expression->op.token_type == TokenType::Plus);
    }
}
