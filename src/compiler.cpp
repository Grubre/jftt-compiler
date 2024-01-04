#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>

#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"

auto load_file(const std::string &filepath) -> std::string {
    const auto file = std::ifstream(filepath);

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

    const auto filepath = std::string(argv[1]);

    const auto source = load_file(filepath);

    auto lexer = Lexer(source);

    auto tokens = std::vector<Token>{};

    bool error_occured = false;

    for (auto &token : lexer) {
        if (token) {
            std::cout << token->lexeme << std::endl;
            tokens.push_back(*token);
        } else {
            display_error(token.error());
            error_occured = true;
        }
    }

    if (error_occured) {
        return 1;
    }

    auto parser = parser::Parser(tokens);

    auto program = parser.parse_program();

    if (program) {
        std::cout << "Program parsed successfully!" << std::endl;
    } else {
        for (auto &error : parser.get_errors()) {
            display_error(error);
        }
        return 1;
    }

    return 0;
}
