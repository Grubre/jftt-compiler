#include "error.hpp"

#include <fmt/color.h>
#include <iostream>

auto Error::operator==(const Error &other) const -> bool {
    return (source == other.source && message == other.message &&
            line == other.line && column == other.column);
}

auto display_error(const Error &error) -> void {
    fmt::print(fg(fmt::color::white) | fmt::emphasis::bold,
               "{}:{}:{}: ", error.source, error.line, error.column);
    fmt::print(fg(fmt::color::red), "Fatal Error: ");
    fmt::print("{}.\n", error.message);
}
