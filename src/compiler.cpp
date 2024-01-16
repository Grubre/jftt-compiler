#include <concepts>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>

#include "analyzer.hpp"
#include "emitter.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "mw-cln.hpp"
#include "parser.hpp"

auto load_file(const std::string &filepath) -> std::string {
    const auto file = std::ifstream(filepath);

    if (!file) {
        std::cerr << "Error: File " << std::quoted(filepath) << " not found."
                  << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct CmdlineArgs {
    std::string input_file;
    std::optional<std::string> output_file;
};

void display_errors(const ThrowsError auto &collection) {
    bool is_error = false;
    const auto errors = collection.get_errors();
    for (auto &error : errors) {
        is_error = display_error(error);
    }
    if (is_error)
        exit(1);
}

auto parse_cmdline_args(int argc, char **argv) -> CmdlineArgs {
    if (argc < 2) {
        std::cerr << "Usage: " + std::string{argv[0]} +
                         " <input_file> [output_file]"
                  << std::endl;
        exit(1);
    }

    const auto input_file = std::string(argv[1]);

    std::optional<std::string> output_file;

    if (argc == 3) {
        output_file = std::string(argv[2]);
    }

    return {input_file, output_file};
}

auto main(int argc, char **argv) -> int {
    const auto args = parse_cmdline_args(argc, argv);

    const auto filepath = std::string(args.input_file);

    const auto source = load_file(filepath);

    auto lexer = Lexer(source);

    auto tokens = std::vector<Token>{};

    bool error_occured = false;

    for (auto &token : lexer) {
        if (token) {
            // std::cout << to_string(*token) << std::endl;
            tokens.push_back(*token);
        } else {
            error_occured = display_error(token.error());
        }
    }

    if (error_occured) {
        return 1;
    }

    auto parser = parser::Parser(tokens);

    auto program = parser.parse_program();

    if (!program) {
        display_errors(parser);
    }

    auto analyzer = analyzer::Analyzer(*program);

    if (!analyzer.analyze()) {
        display_errors(analyzer);
    }

    auto emitter = emitter::Emitter(std::move(*program));

    emitter.emit();

    if (emitter.get_errors().size() > 0) {
        display_errors(emitter);
    }

    if (args.output_file) {
        std::ofstream output(*args.output_file);
        for (auto &line : emitter.get_lines()) {
            const auto instruction = line.instruction;
            const auto comment = line.comment;

            output << to_string(instruction);
            if (!comment.empty())
                output << "\t\t\t\t\t# " << comment;
            output << std::endl;
        }
        return 0;
    }

    auto read_handler = std::make_unique<ReadHandlerStdin>();
    auto write_handler = std::make_unique<WriteHandlerStdout<cln::cl_I>>();

    const auto state = run_machine(emitter.get_lines(), read_handler.get(),
                                   write_handler.get());

    return 0;
}
