#include "debugger-wm.hpp"
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>

std::vector<std::string> split(std::string const &input) {
    std::istringstream buffer(input);
    std::vector<std::string> ret((std::istream_iterator<std::string>(buffer)),
                                 std::istream_iterator<std::string>());
    return ret;
}
auto skip_whitespace(const std::string &str) -> std::size_t {
    auto i = 0u;
    while (i < str.size() && std::isspace(str[i]))
        i++;
    return i;
}

auto parse_lines(const std::vector<std::string> &lines)
    -> std::vector<emitter::Instruction> {
    auto line_number = 1;
    std::vector<emitter::Instruction> instructions{};
    for (const auto &line : lines) {
        const auto tokens = split(line);
        // skip empty lines
        if (tokens.size() == 0) {
            continue;
        }

        // skip comments
        if (tokens[0][0] == '#') {
            continue;
        }

        auto instruction = emitter::mnemonic_from_string(tokens[0]);

        if (instruction == std::nullopt) {
            std::cerr << std::format("Error:{}: Invalid mnemonic '{}'",
                                     line_number, tokens[0])
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // No parameters passed to these instructions
        if (std::holds_alternative<emitter::Read>(*instruction) ||
            std::holds_alternative<emitter::Write>(*instruction) ||
            std::holds_alternative<emitter::Halt>(*instruction)) {
            instructions.push_back(*instruction);
            continue;
        }

        if (std::holds_alternative<emitter::Jump>(*instruction) ||
            std::holds_alternative<emitter::Jpos>(*instruction) ||
            std::holds_alternative<emitter::Jzero>(*instruction)) {
            try {
                const auto instruction_number = std::stoll(tokens[1]);
                emitter::set_jump_location(*instruction, line_number);
                instructions.push_back(*instruction);
            } catch (const std::invalid_argument &) {
                std::cerr << std::format(
                                 "Error:{}: Invalid instruction number '{}'",
                                 line_number, tokens[1])
                          << std::endl;
                std::exit(EXIT_FAILURE);
            }
            continue;
        }

        const auto reg = emitter::from_string(tokens[1]);

        if (!reg) {
            std::cerr << std::format("Error:{}: Invalid register '{}'",
                                     line_number, tokens[1])
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }

        emitter::set_instruction_register(*instruction, *reg);

        instructions.push_back(*instruction);

        line_number++;
    }

    return instructions;
}

VirtualMachine::VirtualMachine(std::vector<emitter::Instruction> instructions)
    : instructions(std::move(instructions)) {

    lr = 0;
    srand(time(NULL));
    for (int i = 0; i < 8; i++)
        r[i] = rand();
    io = 0;
}
