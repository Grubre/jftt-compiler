#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace instruction {

enum class Register { A, B, C, D, E, F, G, H };

auto to_string(Register reg) -> std::string;
auto from_string(const std::string &reg) -> std::optional<Register>;

struct Read {};
struct Write {};

struct Load {
    Register address;
};
struct Store {
    Register address;
};
struct Add {
    Register address;
};
struct Sub {
    Register address;
};
struct Get {
    Register address;
};
struct Put {
    Register address;
};
struct Rst {
    Register address;
};
struct Inc {
    Register address;
};
struct Dec {
    Register address;
};
struct Shl {
    Register address;
};
struct Shr {
    Register address;
};

struct Jump {
    uint64_t line;
};
struct Jpos {
    uint64_t line;
};
struct Jzero {
    uint64_t line;
};

struct Strk {
    Register reg;
};
struct Jumpr {
    Register reg;
};

struct Halt {};

struct Comment {
    std::string comment;
    uint64_t indent = 8;

    auto get_str() const -> std::string {
        std::string str = "";
        for (auto i = 0u; i < indent; i++)
            str += " ";
        return str + comment;
    }
};
using Instruction = std::variant<Read, Write, Load, Store, Add, Sub, Get, Put, Rst, Inc, Dec, Shl, Shr, Jump, Jpos,
                                 Jzero, Strk, Jumpr, Halt, Comment>;

auto to_string(const Instruction &instruction) -> std::string;
auto mnemonic_from_string(const std::string &instruction) -> std::optional<Instruction>;
void set_jump_location(Instruction &instruction, uint64_t location);
void set_instruction_register(Instruction &instruction, Register reg);

struct Line {
    Instruction instruction;
    std::string comment{};
};

} // namespace instruction
