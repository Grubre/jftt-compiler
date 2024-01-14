#pragma once
#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

enum class StateCode {
    RUNNING,
    HALTED,
    PENDING_INPUT,
    PENDING_OUTPUT,
    ERROR,
};

auto parse_lines(const std::vector<std::string> &lines)
    -> std::vector<emitter::Instruction>;

struct VirtualMachine {
    VirtualMachine(std::vector<emitter::Instruction> instructions);

    auto process_next_instruction() -> StateCode;
    auto get_output() -> long long;
    void set_input(long long input);

    std::vector<emitter::Instruction> instructions;
    std::array<long long, 8> r;
    std::map<long long, long long> pam;
    long long lr;
    long long io = 0;
    long long t = 0;
};
