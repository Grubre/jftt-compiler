#pragma once

#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <emitter.hpp>
#include <map>

struct ProgramState {
    std::vector<uint64_t> outputs;
    std::array<long long, 8> r;
    std::map<long long, long long> pam;
    bool error;
};

ProgramState run_machine(const std::vector<emitter::Line> &lines,
                         std::deque<uint64_t> input_values);
