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
    Program,
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

constexpr auto to_string(TokenType token_type) -> std::string {
    switch (token_type) {
    case Pidentifier:
        return "Pidentifier";
    case Num:
        return "Num";
    case Plus:
        return "Plus";
    case Minus:
        return "Minus";
    case Star:
        return "Star";
    case Slash:
        return "Slash";
    case Percent:
        return "Percent";
    case Equals:
        return "Equals";
    case BangEquals:
        return "BangEquals";
    case Greater:
        return "Greater";
    case GreaterEquals:
        return "GreaterEquals";
    case Less:
        return "Less";
    case LessEquals:
        return "LessEquals";
    case Lparen:
        return "Lparen";
    case Rparen:
        return "Rparen";
    case Lbracket:
        return "Lbracket";
    case Rbracket:
        return "Rbracket";
    case Walrus:
        return "Walrus";
    case Semicolon:
        return "Semicolon";
    case Program:
        return "Program";
    case Procedure:
        return "Procedure";
    case Is:
        return "Is";
    case In:
        return "In";
    case While:
        return "While";
    case EndWhile:
        return "EndWhile";
    case If:
        return "If";
    case EndIf:
        return "EndIf";
    case Then:
        return "Then";
    case Else:
        return "Else";
    case Do:
        return "Do";
    case Read:
        return "Read";
    case Write:
        return "Write";
    case End:
        return "End";
    case Comma:
        return "Comma";
    }
}

struct Token {
    TokenType token_type;
    std::string lexeme;
    unsigned line;
    unsigned column;

    auto operator==(const Token &other) const -> bool {
        return token_type == other.token_type && lexeme == other.lexeme &&
               line == other.line && column == other.column;
    }
};
