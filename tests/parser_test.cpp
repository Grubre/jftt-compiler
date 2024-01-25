#include "ast.hpp"
#include <format>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "parser.hpp"
#include "tests_shared.hpp"
#include "token.hpp"
#include <array>
#include <variant>

// TODO: Add tests for the remaining parser methods

auto make_pidentifier(std::string name) -> Token {
    return Token{.token_type = TokenType::Pidentifier, .lexeme = std::move(name), .line = 1, .column = 1};
}

auto make_comma() -> Token { return Token{.token_type = TokenType::Comma, .lexeme = ",", .line = 1, .column = 1}; }

auto make_num(std::string value) -> Token {
    return Token{.token_type = TokenType::Num, .lexeme = std::move(value), .line = 1, .column = 1};
}

auto make_num_or_pidentifier(std::string value) -> Token {
    if (value[0] >= '0' && value[0] <= '9') {
        return make_num(std::move(value));
    } else {
        return make_pidentifier(std::move(value));
    }
}

auto make_lbracket() -> Token {
    return Token{.token_type = TokenType::Lbracket, .lexeme = "[", .line = 1, .column = 1};
}

auto make_rbracket() -> Token {
    return Token{.token_type = TokenType::Rbracket, .lexeme = "]", .line = 1, .column = 1};
}

auto make_operator(char op) -> Token {
    switch (op) {
    case '+':
        return Token{.token_type = TokenType::Plus, .lexeme = std::string{op}, .line = 1, .column = 1};
    case '-':
        return Token{.token_type = TokenType::Minus, .lexeme = std::string{op}, .line = 1, .column = 1};
    case '*':
        return Token{.token_type = TokenType::Star, .lexeme = std::string{op}, .line = 1, .column = 1};
    case '/':
        return Token{.token_type = TokenType::Slash, .lexeme = std::string{op}, .line = 1, .column = 1};
    case '%':
        return Token{.token_type = TokenType::Percent, .lexeme = std::string{op}, .line = 1, .column = 1};
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
    auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(), make_pidentifier("m")};
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
    auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(), make_pidentifier("m")};
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
    auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(), make_pidentifier("m")};
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
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_comma(), make_pidentifier("m")};
        auto parser = parser::Parser(tokens);

        const auto declarations = parser.parse_declarations();
        CHECK(declarations.has_value());
        CHECK(declarations->size() == 2);
        CHECK(declarations->front().identifier.lexeme == "n");
        CHECK(declarations->back().identifier.lexeme == "m");
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
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_lbracket(), make_num("0"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto identifier = parser.parse_identifier();
        CHECK(identifier.has_value());
        CHECK(identifier->name.lexeme == "n");
        CHECK(identifier->index.has_value());
        CHECK(identifier->index->lexeme == "0");
    }

    SUBCASE("Pidentifier index") {
        auto tokens =
            std::vector<Token>{make_pidentifier("n"), make_lbracket(), make_pidentifier("m"), make_rbracket()};
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
        CHECK(std::holds_alternative<ast::Num>(*value));
        CHECK(std::get<ast::Num>(*value).lexeme == "0");
    }

    SUBCASE("Identifier") {
        auto tokens = std::vector<Token>{make_pidentifier("n")};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<ast::Identifier>(*value));
        CHECK(std::get<ast::Identifier>(*value).name.lexeme == "n");
        CHECK(!std::get<ast::Identifier>(*value).index.has_value());
    }

    SUBCASE("Identifier indexed by identifier") {
        auto tokens =
            std::vector<Token>{make_pidentifier("n"), make_lbracket(), make_pidentifier("m"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<ast::Identifier>(*value));
        CHECK(std::get<ast::Identifier>(*value).name.lexeme == "n");
        CHECK(std::get<ast::Identifier>(*value).index.has_value());
        CHECK(std::get<ast::Identifier>(*value).index->lexeme == "m");
    }

    SUBCASE("Identifier indexed by num") {
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_lbracket(), make_num("0"), make_rbracket()};
        auto parser = parser::Parser(tokens);

        const auto value = parser.parse_value();
        CHECK(value.has_value());
        CHECK(std::holds_alternative<ast::Identifier>(*value));
        CHECK(std::get<ast::Identifier>(*value).name.lexeme == "n");
        CHECK(std::get<ast::Identifier>(*value).index.has_value());
        CHECK(std::get<ast::Identifier>(*value).index->lexeme == "0");
    }
}

TEST_CASE("Parser - parse expression") {
    char op;
    auto operators = std::array{'+', '-', '*', '/', '%'};

    auto operator_to_tokentype = [](char op) -> TokenType {
        switch (op) {
        case '+':
            return TokenType::Plus;
        case '-':
            return TokenType::Minus;
        case '*':
            return TokenType::Star;
        case '/':
            return TokenType::Slash;
        case '%':
            return TokenType::Percent;
        }
        assert(false);
    };

    SUBCASE("Num Num") {
        DOCTEST_VALUE_PARAMETERIZED_DATA(op, operators);
        auto tokens = std::vector<Token>{make_num("0"), make_operator(op), make_num("1")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        const auto bin = std::get<ast::BinaryExpression>(*expression);

        CHECK(expression.has_value());
        CHECK(std::holds_alternative<ast::Num>(bin.lhs));
        CHECK(std::holds_alternative<ast::Num>(bin.rhs));
        CHECK(bin.op.token_type == operator_to_tokentype(op));
    }

    SUBCASE("Num Identifier") {
        DOCTEST_VALUE_PARAMETERIZED_DATA(op, operators);
        auto tokens = std::vector<Token>{make_num("0"), make_operator(op), make_pidentifier("n")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        const auto bin = std::get<ast::BinaryExpression>(*expression);

        CHECK(expression.has_value());
        CHECK(std::holds_alternative<ast::Num>(bin.lhs));
        CHECK(std::holds_alternative<ast::Identifier>(bin.rhs));
        CHECK(bin.op.token_type == operator_to_tokentype(op));
    }

    SUBCASE("Identifier Num") {
        DOCTEST_VALUE_PARAMETERIZED_DATA(op, operators);
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_operator(op), make_num("0")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        const auto bin = std::get<ast::BinaryExpression>(*expression);

        CHECK(expression.has_value());
        CHECK(std::holds_alternative<ast::Identifier>(bin.lhs));
        CHECK(std::holds_alternative<ast::Num>(bin.rhs));
        CHECK(bin.op.token_type == operator_to_tokentype(op));
    }

    SUBCASE("Identifier Identifier") {
        DOCTEST_VALUE_PARAMETERIZED_DATA(op, operators);
        auto tokens = std::vector<Token>{make_pidentifier("n"), make_operator(op), make_pidentifier("m")};
        auto parser = parser::Parser(tokens);

        const auto expression = parser.parse_expression();
        const auto bin = std::get<ast::BinaryExpression>(*expression);

        CHECK(expression.has_value());
        CHECK(std::holds_alternative<ast::Identifier>(bin.lhs));
        CHECK(std::holds_alternative<ast::Identifier>(bin.rhs));
        CHECK(bin.op.token_type == operator_to_tokentype(op));
    }
}
