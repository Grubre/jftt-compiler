#include "lexer.hpp"
#include "error.hpp"
#include "expected.hpp"

#include <format>
#include <iostream>
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
    {"PROGRAM", TokenType::Program}, {"PROCEDURE", TokenType::Procedure},
    {"IS", TokenType::Is},           {"IN", TokenType::In},
    {"WHILE", TokenType::While},     {"ENDWHILE", TokenType::EndWhile},
    {"IF", TokenType::If},           {"ENDIF", TokenType::EndIf},
    {"THEN", TokenType::Then},       {"ELSE", TokenType::Else},
    {"DO", TokenType::Do},           {"READ", TokenType::Read},
    {"WRITE", TokenType::Write},     {"END", TokenType::End}};

Lexer::Lexer(const std::string &source) : source(source) {
    line_number = 1;
    column_number = 1;
    current_index = 0;
}

auto Lexer::newline() -> void {
    line_number++;
    column_number = 1;
}

auto Lexer::make_token(TokenType token_type, std::string lexeme) -> Token {
    return Token{token_type, lexeme, line_number, column_number};
}

auto Lexer::chop(int count) -> std::string {
    auto result = source.substr(current_index, count);
    current_index += count;
    column_number += count;
    return result;
}

auto Lexer::chop_while(std::function<bool(char)> predicate) -> std::string {
    std::string result;
    while (current_index < source.size() && predicate(source[current_index])) {
        char c = source[current_index];
        result += c;
        current_index++;
        column_number++;

        if (c == '\n') {
            newline();
        }
    }

    return result;
}

auto Lexer::peek() -> std::optional<char> {
    if (current_index + 1 >= source.size()) {
        return std::nullopt;
    }

    return source[current_index + 1];
}

void Lexer::trim_whitespace() {
    while (current_index < source.size()) {
        char c = source[current_index];
        if (c == ' ' || c == '\t' || c == '\n') {
            current_index++;
            column_number++;

            if (c == '\n')
                newline();
        } else {
            break;
        }
    }
}

auto is_numeric(char c) -> bool { return c >= '0' && c <= '9'; }

auto is_alphabetic(char c) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

auto Lexer::next_token() -> std::optional<tl::expected<Token, Error>> {
    trim_whitespace();

    if (current_index >= source.size()) {
        return std::nullopt;
    }

    char c = source[current_index];

    if (c == '#') {
        chop_while([](char c) { return c != '\n'; });
        return next_token();
    }

    if (is_numeric(c)) {
        auto lexeme = chop_while(is_numeric);
        return make_token(TokenType::Num, lexeme);
    }

    if (is_alphabetic(c) || c == '_') {
        auto lexeme = chop_while([](char c) {
            return is_alphabetic(c) || is_numeric(c) || c == '_';
        });
        auto keyword = keywords.find(lexeme);
        if (keyword != keywords.end()) {
            return make_token(keyword->second, lexeme);
        } else {
            return make_token(TokenType::Pidentifier, lexeme);
        }
    }

    switch (c) {
    case '+':
        return make_token(TokenType::Plus, chop(1));
    case '-':
        return make_token(TokenType::Minus, chop(1));
    case '*':
        return make_token(TokenType::Star, chop(1));
    case '/':
        return make_token(TokenType::Slash, chop(1));
    case '%':
        return make_token(TokenType::Percent, chop(1));
    case '=':
        return make_token(TokenType::Equals, chop(1));
    case '!':
        if (peek() == '=') {
            return make_token(TokenType::BangEquals, chop(2));
        } else {
            return tl::unexpected(Error{
                "Lexer",
                std::format("Unexpected character: Expected '!=', found '!{}'",
                            chop(1)),
                line_number, column_number});
        }
    case '>':
        if (peek() == '=') {
            return make_token(TokenType::GreaterEquals, chop(2));
        } else {
            return make_token(TokenType::Greater, chop(1));
        }
    case '<':
        if (peek() == '=') {
            return make_token(TokenType::LessEquals, chop(2));
        } else {
            return make_token(TokenType::Less, chop(1));
        }
    case '(':
        return make_token(TokenType::Lparen, chop(1));
    case ')':
        return make_token(TokenType::Rparen, chop(1));
    case '[':
        return make_token(TokenType::Lbracket, chop(1));
    case ']':
        return make_token(TokenType::Rbracket, chop(1));
    case ':':
        if (peek() == '=') {
            return make_token(TokenType::Walrus, chop(2));
        } else {
            return tl::unexpected(Error{
                "Lexer",
                std::format("Unexpected character: Expected ':=', found ':{}'",
                            chop(1)),
                line_number, column_number});
        }
    case ';':
        return make_token(TokenType::Semicolon, chop(1));
    case ',':
        return make_token(TokenType::Comma, chop(1));
    }

    return tl::unexpected(Error{"Lexer",
                                std::format("Unexpected character '{}'", c),
                                line_number, column_number});
}
