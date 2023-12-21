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
    End
};

struct Token {
    TokenType toket_type;
    std::string lexeme;
    unsigned line;
    unsigned column;
};
