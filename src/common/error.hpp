#pragma once

#include <string>

struct Error {
    std::string source;
    std::string message;
    unsigned line;
    unsigned column;

    auto operator==(const Error &other) const -> bool;
};

auto display_error(const Error &error) -> void;
