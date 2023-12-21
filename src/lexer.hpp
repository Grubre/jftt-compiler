#pragma once

#include <unordered_map>

#include "token.hpp"
#include "expected.hpp"
#include "error.hpp"
#include <optional>

class Lexer {
public:
    Lexer(const std::string& source);
    auto next_token() -> std::optional<tl::expected<Token, Error>>;

private:
    unsigned current_line = 0;
    std::string source;
};
