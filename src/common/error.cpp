#include "error.hpp"

#include <fmt/color.h>
#include <iostream>

auto Error::operator==(const Error &other) const -> bool {
    return (source == other.source && message == other.message && line == other.line && column == other.column &&
            is_warning == other.is_warning);
}

auto display_error(const Error &error) -> bool {
    fmt::print(fg(fmt::color::white) | fmt::emphasis::bold, "{}:{}:{}: ", error.source, error.line, error.column);
    if (error.is_warning) {
        fmt::print(fg(fmt::color::yellow), "Warning: ");
    } else {
        fmt::print(fg(fmt::color::red), "Fatal Error: ");
    }
    fmt::print("{}.\n", error.message);

    return !error.is_warning;
}
