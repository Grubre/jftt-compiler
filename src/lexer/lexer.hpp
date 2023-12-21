#pragma once

#include "token.hpp"
#include "expected.hpp"
#include "error.hpp"
#include <optional>
#include <functional>

class Lexer {
public:
    Lexer(const std::string& source);
    auto next_token() -> std::optional<Token>;

    class Iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = Token;
        using difference_type = std::ptrdiff_t;
        using pointer = std::optional<Token>;
        using reference = Token&;
    public:
        Iterator(Lexer* lexer = nullptr, pointer currentToken = std::nullopt)
            : lexer(lexer), currentToken(std::move(currentToken)) {}

        reference operator*() { return *currentToken; }
        pointer operator->() { return currentToken; }

        Iterator& operator++() {
            if (lexer) {
                currentToken = lexer->next_token();
                if (!currentToken) {
                    lexer = nullptr;
                }
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return (lexer == other.lexer && currentToken == other.currentToken);
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        Lexer* lexer;
        std::optional<Token> currentToken;
    };

    Iterator begin() {
        return Iterator(this, next_token());
    }

    Iterator end() {
        return Iterator();
    }

private:
    auto chop(int count) -> std::string;
    auto chop_while(std::function<bool(char)>) -> std::string;
    auto peek() -> std::optional<char>;
    void trim_whitespace();
    void newline();
    auto make_token(TokenType token_type, std::string lexeme) -> Token;

private:
    unsigned line_number;
    unsigned column_number;
    unsigned current_index;
    std::string source;
};
