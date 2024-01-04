#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "parser.hpp"
#include "tests_shared.hpp"
#include "token.hpp"

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
