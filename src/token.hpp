#pragma once

#include <string>
enum TokenType {
    Pidentifier,
    Num,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equals,
    BangEquals,
    Greater,
    GreaterEquals,
    Less,
    LessEquals,
    Lparen,
    Rparen,
    Lbracket,
    Rbracket,
    Walrus,
    Semicolon,
    Colon,
    Procedure,
    Is,
    In,
    While,
    EndWhile,
    If,
    EndIf,
    Then,
    Else,
    Do,
    Read,
    Write,
    End,
    Comma
};

struct Token {
    TokenType token_type;
    std::string lexeme;
    unsigned line;
    unsigned column;

    auto operator==(const Token& other) const -> bool {
        return token_type == other.token_type && lexeme == other.lexeme && line == other.line && column == other.column;
    }
};
