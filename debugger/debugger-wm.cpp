#include "debugger-wm.hpp"
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>

auto split(std::string const &input) -> std::vector<std::string> {
    std::istringstream buffer(input);
    std::vector<std::string> ret((std::istream_iterator<std::string>(buffer)), std::istream_iterator<std::string>());
    return ret;
}

auto skip_whitespace(const std::string &str) -> std::size_t {
    auto i = 0u;
    while (i < str.size() && std::isspace(str[i]))
        i++;
    return i;
}

auto parse_lines(const std::vector<std::string> &lines) -> std::vector<instruction::Instruction> {
    auto line_number = 1;
    std::vector<instruction::Instruction> instructions{};
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

        auto instruction = instruction::mnemonic_from_string(tokens[0]);

        if (instruction == std::nullopt) {
            std::cerr << std::format("Error:{}: Invalid mnemonic '{}'", line_number, tokens[0]) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // No parameters passed to these instructions
        if (std::holds_alternative<instruction::Read>(*instruction) ||
            std::holds_alternative<instruction::Write>(*instruction) ||
            std::holds_alternative<instruction::Halt>(*instruction)) {
            instructions.push_back(*instruction);
            continue;
        }

        if (std::holds_alternative<instruction::Jump>(*instruction) ||
            std::holds_alternative<instruction::Jpos>(*instruction) ||
            std::holds_alternative<instruction::Jzero>(*instruction)) {
            try {
                const auto instruction_number = std::stoll(tokens[1]);
                instruction::set_jump_location(*instruction, (unsigned int)instruction_number);
                instructions.push_back(*instruction);
            } catch (const std::invalid_argument &) {
                std::cerr << std::format("Error:{}: Invalid instruction number '{}'", line_number, tokens[1])
                          << std::endl;
                std::exit(EXIT_FAILURE);
            }
            continue;
        }

        const auto reg = instruction::from_string(tokens[1]);

        if (!reg) {
            std::cerr << std::format("Error:{}: Invalid register '{}'", line_number, tokens[1]) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        instruction::set_instruction_register(*instruction, *reg);

        instructions.push_back(*instruction);

        line_number++;
    }

    return instructions;
}

VirtualMachine::VirtualMachine(std::vector<instruction::Instruction> instructions)
    : instructions(std::move(instructions)) {
    lr = 0;
    srand((unsigned int)time(NULL));
    for (auto i = 0u; i < 8; i++)
        r[i] = rand();
    io = 0;
    t = 0;
}

auto VirtualMachine::get_output() -> long long {
    io += 100;
    lr++;
    return r[0];
}

void VirtualMachine::set_input(long long input) {
    r[0] = input;
    io += 100;
    lr++;
}

auto VirtualMachine::process_next_instruction() -> StateCode {
    auto state_code = StateCode::RUNNING;
    std::visit(overloaded{[&](const instruction::Read &) { state_code = StateCode::PENDING_INPUT; },
                          [&](const instruction::Write &) { state_code = StateCode::PENDING_OUTPUT; },
                          [&](const instruction::Load &load) {
                              r[0] = pam[r[(unsigned int)load.address]];
                              t += 50;
                              lr++;
                          },
                          [&](const instruction::Store &store) {
                              pam[r[(unsigned int)store.address]] = r[0];
                              t += 50;
                              lr++;
                          },
                          [&](const instruction::Add &add) {
                              r[0] += r[(unsigned int)add.address];
                              t += 5;
                              lr++;
                          },
                          [&](const instruction::Sub &sub) {
                              r[0] -= r[0] >= r[(unsigned int)sub.address] ? r[(unsigned int)sub.address] : r[0];
                              t += 5;
                              lr++;
                          },
                          [&](const instruction::Get &get) {
                              r[0] = r[(unsigned int)get.address];
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Put &put) {
                              r[(unsigned int)put.address] = r[0];
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Rst &rst) {
                              r[(unsigned int)rst.address] = 0;
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Inc &inc) {
                              r[(unsigned int)inc.address]++;
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Dec &dec) {
                              if (r[(unsigned int)dec.address] > 0)
                                  r[(unsigned int)dec.address]--;
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Shl &shl) {
                              r[(unsigned int)shl.address] <<= 1;
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Shr &shr) {
                              r[(unsigned int)shr.address] >>= 1;
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Jump &jump) {
                              lr = (long long)jump.line;
                              t += 1;
                          },
                          [&](const instruction::Jpos &jpos) {
                              if (r[0] > 0)
                                  lr = (long long)jpos.line;
                              else
                                  lr++;
                              t += 1;
                          },
                          [&](const instruction::Jzero &jzero) {
                              if (r[0] == 0)
                                  lr = (long long)jzero.line;
                              else
                                  lr++;
                              t += 1;
                          },
                          [&](const instruction::Strk &strk) {
                              r[(unsigned int)strk.reg] = lr;
                              t += 1;
                              lr++;
                          },
                          [&](const instruction::Jumpr &jumpr) {
                              lr = r[(unsigned int)jumpr.reg];
                              t += 1;
                          },
                          [&](const instruction::Halt &) { state_code = StateCode::HALTED; },
                          [&](const instruction::Comment &) {}},
               instructions[(unsigned long)lr]);

    if (lr < 0 || lr >= (unsigned int)instructions.size()) {
        return StateCode::ERROR;
    }

    return state_code;
}
