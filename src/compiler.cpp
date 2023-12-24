#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>

#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"

auto load_file(const std::string &filepath) -> std::string {
    auto file = std::ifstream(filepath);

    if (!file) {
        std::cerr << "Error: File " << std::quoted(filepath) << " not found."
                  << std::endl;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

auto main(int argc, char **argv) -> int {
    if (argc != 2) {
        std::cout << "Usage: " + std::string{argv[0]} + " <input_file>"
                  << std::endl;
        return 1;
    }

    auto filepath = std::string(argv[1]);

    auto source = load_file(filepath);

    auto lexer = Lexer(source);

    for (auto &token : lexer) {
        if (token)
            std::cout << token->lexeme << std::endl;
        else
            display_error(token.error());
    }

    return 0;
}
