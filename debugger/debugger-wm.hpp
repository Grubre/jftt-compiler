#pragma once
#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
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
    VirtualMachine();
    StateCode process_next_instruction();

    long long get_output() const;
    void set_input(long long input);

    std::array<long long, 8> r;
    std::unordered_map<long long, long long> pam;
    long long lr;
    long long io;
};
