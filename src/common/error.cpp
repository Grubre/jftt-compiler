#include "error.hpp"

#include <iostream>

[[noreturn]] auto throw_error(const Error& error) -> void {
    std::cerr << "Error(" << error.source << "):" << error.line << ":" << error.column << ": " << error.message << std::endl;
    exit(1);
}
