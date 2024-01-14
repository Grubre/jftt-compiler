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
                emitter::set_jump_location(*instruction, instruction_number);
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
    std::visit(overloaded{[&](const emitter::Read &) {
                              state_code = StateCode::PENDING_INPUT;
                          },
                          [&](const emitter::Write &) {
                              state_code = StateCode::PENDING_OUTPUT;
                          },
                          [&](const emitter::Load &load) {
                              r[0] = pam[r[(int)load.address]];
                              t += 50;
                              lr++;
                          },
                          [&](const emitter::Store &store) {
                              pam[r[(int)store.address]] = r[0];
                              t += 50;
                              lr++;
                          },
                          [&](const emitter::Add &add) {
                              r[0] += r[(int)add.address];
                              t += 5;
                              lr++;
                          },
                          [&](const emitter::Sub &sub) {
                              r[0] -= r[0] >= r[(int)sub.address]
                                          ? r[(int)sub.address]
                                          : r[0];
                              t += 5;
                              lr++;
                          },
                          [&](const emitter::Get &get) {
                              r[0] = r[(int)get.address];
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Put &put) {
                              r[(int)put.address] = r[0];
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Rst &rst) {
                              r[(int)rst.address] = 0;
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Inc &inc) {
                              r[(int)inc.address]++;
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Dec &dec) {
                              if (r[(int)dec.address] > 0)
                                  r[(int)dec.address]--;
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Shl &shl) {
                              r[(int)shl.address] <<= 1;
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Shr &shr) {
                              r[(int)shr.address] >>= 1;
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Jump &jump) {
                              lr = jump.line;
                              t += 1;
                          },
                          [&](const emitter::Jpos &jpos) {
                              if (r[0] > 0)
                                  lr = jpos.line;
                              else
                                  lr++;
                              t += 1;
                          },
                          [&](const emitter::Jzero &jzero) {
                              if (r[0] == 0)
                                  lr = jzero.line;
                              else
                                  lr++;
                              t += 1;
                          },
                          [&](const emitter::Strk &strk) {
                              r[(int)strk.reg] = lr;
                              t += 1;
                              lr++;
                          },
                          [&](const emitter::Jumpr &jumpr) {
                              lr = r[(int)jumpr.reg];
                              t += 1;
                          },
                          [&](const emitter::Halt &) {
                              state_code = StateCode::HALTED;
                          },
                          [&](const emitter::Comment &comment) {}},
               instructions[lr]);

    if (lr < 0 || lr >= (int)instructions.size()) {
        return StateCode::ERROR;
    }

    return state_code;
}
