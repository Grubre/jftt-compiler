#include <random>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "lexer.hpp"
#include "tests_shared.hpp"
#include "token.hpp"
#include <array>

constexpr std::array keywords = {"PROCEDURE", "IS",   "IN",   "WHILE", "ENDWHILE", "IF",
                                 "ENDIF",     "THEN", "ELSE", "DO",    "READ",     "WRITE"};

TEST_CASE("Lexer - keywords") {
    constexpr std::array expected_types = {TokenType::Procedure, TokenType::Is, TokenType::In,    TokenType::While,
                                           TokenType::EndWhile,  TokenType::If, TokenType::EndIf, TokenType::Then,
                                           TokenType::Else,      TokenType::Do, TokenType::Read,  TokenType::Write};

    for (auto &keyword : keywords) {
        auto lexer = Lexer(keyword);
        auto token = lexer.next_token();
        CHECK(token.has_value());
        CHECK(token->has_value());
        CHECK(token->value().token_type == expected_types[&keyword - &keywords[0]]);
        CHECK(token->value().lexeme == keyword);
    }
}

auto generate_random_pidentifier() -> std::string {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(1, 20);
    static std::uniform_int_distribution<> char_dist(0, 26);

    auto length = dist(gen);
    std::string word;
    word.reserve(length);

    auto random_char = []() -> char {
        auto char_index = char_dist(gen);
        return char_index < 26 ? 'a' + (char)char_index : '_';
    };

    std::ranges::generate_n(std::back_inserter(word), length, random_char);

    return word;
}

TEST_CASE("Lexer - identifiers [_a-z]+") {
    std::vector<std::string> identifiers;
    for (int i = 0; i < 10000; i++) {
        auto identifier = generate_random_pidentifier();
        if (std::ranges::find(keywords, identifier) != keywords.end()) {
            i--;
            continue;
        }
        identifiers.push_back(identifier);
    }

    for (auto &identifier : identifiers) {
        auto lexer = Lexer(identifier);
        auto token = lexer.next_token();
        CHECK(token.has_value());
        CHECK(token->has_value());
        CHECK(token->value().token_type == TokenType::Pidentifier);
        CHECK(token->value().lexeme == identifier);
    }
}

TEST_CASE("Lexer - numbers") {
    std::vector<std::string> numbers;
    for (int i = 0; i < 10000; i++) {
        auto number = std::to_string(i);
        numbers.push_back(number);
    }

    for (auto &number : numbers) {
        auto lexer = Lexer(number);
        auto token = lexer.next_token();
        CHECK(token.has_value());
        CHECK(token->has_value());
        CHECK(token->value().token_type == TokenType::Num);
        CHECK(token->value().lexeme == number);
    }
}

TEST_CASE("Lexer - single character tokens") {
    constexpr std::array expected_types = {
        TokenType::Plus,     TokenType::Minus,    TokenType::Star,      TokenType::Slash,  TokenType::Percent,
        TokenType::Equals,   TokenType::Greater,  TokenType::Less,      TokenType::Lparen, TokenType::Rparen,
        TokenType::Lbracket, TokenType::Rbracket, TokenType::Semicolon, TokenType::Comma};

    constexpr std::array chars = {'+', '-', '*', '/', '%', '=', '>', '<', '(', ')', '[', ']', ';', ','};

    for (auto i = 0u; i < chars.size(); i++) {
        const auto c = chars[i];
        auto lexer = Lexer(std::string(1, c));
        const auto token = lexer.next_token();
        CHECK(token.has_value());
        CHECK(token->value().token_type == expected_types[i]);
        CHECK(token->value().lexeme == std::string(1, c));
    }
}

TEST_CASE("Lexer - double character tokens") {
    constexpr std::array expected_types = {TokenType::BangEquals, TokenType::GreaterEquals, TokenType::LessEquals,
                                           TokenType::Walrus};

    constexpr std::array chars = {'!', '>', '<', ':'};

    for (auto i = 0u; i < chars.size(); i++) {
        const auto c = chars[i];
        const auto expected = std::string{c} + "=";
        auto lexer = Lexer(expected);
        auto token = lexer.next_token();
        CHECK(token.has_value());
        CHECK(token->value().token_type == expected_types[i]);
        CHECK(token->value().lexeme == expected);
    }
}

TEST_CASE("Lexer - comments") {
    const auto comment = "   # a + b; <= This is a comment\n";
    auto lexer = Lexer(comment);
    auto token = lexer.next_token();
    CHECK(!token.has_value());
}

TEST_CASE("Lexer - Provided examples tests - check successful lexing") {
    auto filecontent = read_test_files();
    auto lexer = Lexer(filecontent);

    for (auto &token : lexer) {
        CHECK(token.has_value());
        if (!token) {
            display_error(token.error());
        } else {
        }
    }
}
