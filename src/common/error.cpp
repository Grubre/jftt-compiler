#include "error.hpp"

#include <iostream>

auto Error::operator==(const Error &other) const -> bool {
    return (source == other.source && message == other.message &&
            line == other.line && column == other.column);
}

auto display_error(const Error &error) -> void {
    std::cerr << "Error(" << error.source << "):" << error.line << ":"
              << error.column << ": " << error.message << std::endl;
}
