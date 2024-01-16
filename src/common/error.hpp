#pragma once

#include <string>

struct Error {
    std::string source;
    std::string message;
    unsigned line;
    unsigned column;

    bool is_warning = false;

    auto operator==(const Error &other) const -> bool;
};

// true if error, false if warning
auto display_error(const Error &error) -> bool;
