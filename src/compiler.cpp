#include <iomanip>
#include <iostream>
#include <optional>
#include <fstream>

#include "lexer.hpp"

auto load_file(const std::string& filepath) -> std::string {
    auto file = std::ifstream(filepath);

    if(!file) {
        std::cerr << "Error: File " << std::quoted(filepath) << " not found." << std::endl;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

auto main(int argc, char** argv) -> int {
    if (argc != 2) {
        std::cout << "Usage: " + std::string{argv[0]} + " <input_file>" << std::endl;
        return 1;
    }

    auto filepath = std::string(argv[1]);

    auto source = load_file(filepath);

    auto lexer = Lexer(source);

    for(auto& token : lexer) {
        std::cout << token.lexeme << std::endl;;
    }

    return 0;
}
