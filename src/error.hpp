#pragma once

#include <string>

struct Error {
    std::string source;
    std::string message;
    unsigned line;
    unsigned column;
};

[[noreturn]] auto throw_error(const Error& error) -> void;
