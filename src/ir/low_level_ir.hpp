#pragma once
#include "instruction.hpp"
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lir {
using VirtualRegister = uint64_t;

constexpr auto regA = VirtualRegister{0};

struct Read {};
struct Write {};

struct Load {
    VirtualRegister address;
};
struct Store {
    VirtualRegister address;
};
struct Add {
    VirtualRegister address;
};
struct Sub {
    VirtualRegister address;
};
struct Get {
    VirtualRegister address;
};
struct Put {
    VirtualRegister address;
};
struct Rst {
    VirtualRegister address;
};
struct Inc {
    VirtualRegister address;
};
struct Dec {
    VirtualRegister address;
};
struct Shl {
    VirtualRegister address;
};
struct Shr {
    VirtualRegister address;
};

struct Jump {
    std::string label;
    bool jumps_to_procedure = false;
};
struct Jpos {
    std::string label;
};
struct Jzero {
    std::string label;
};

struct Strk {
    VirtualRegister reg;
};
struct Jumpr {
    VirtualRegister reg;
};

struct Halt {};

struct Label {
    std::string name;
};

using VirtualInstruction = std::variant<Read, Write, Load, Store, Add, Sub, Get, Put, Rst, Inc, Dec, Shl, Shr, Jump,
                                        Jpos, Jzero, Strk, Jumpr, Label, Halt>;

auto read_variables(const VirtualInstruction &instr) -> std::vector<VirtualRegister>;
auto overwritten_variables(const VirtualInstruction &instr) -> std::vector<VirtualRegister>;

auto to_string(const VirtualInstruction &instr) -> std::string;
} // namespace lir
