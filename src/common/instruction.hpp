#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

template <class> inline constexpr bool always_false_v = false;

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace instruction {

enum class Register { A, B, C, D, E, F, G, H };

inline auto to_string(Register reg) -> std::string {
    switch (reg) {
    case Register::A:
        return "a";
    case Register::B:
        return "b";
    case Register::C:
        return "c";
    case Register::D:
        return "d";
    case Register::E:
        return "e";
    case Register::F:
        return "f";
    case Register::G:
        return "g";
    case Register::H:
        return "h";
    }
    assert(false);
}

inline auto from_string(const std::string &reg) -> std::optional<Register> {
    if (reg == "a")
        return Register::A;
    if (reg == "b")
        return Register::B;
    if (reg == "c")
        return Register::C;
    if (reg == "d")
        return Register::D;
    if (reg == "e")
        return Register::E;
    if (reg == "f")
        return Register::F;
    if (reg == "g")
        return Register::G;
    if (reg == "h")
        return Register::H;

    return std::nullopt;
}

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
using Instruction =
    std::variant<Read, Write, Load, Store, Add, Sub, Get, Put, Rst, Inc, Dec,
                 Shl, Shr, Jump, Jpos, Jzero, Strk, Jumpr, Halt, Comment>;

inline auto to_string(const Instruction &instruction) -> std::string {
    return std::visit(
        overloaded{[](const Read &) -> std::string { return "READ"; },
                   [](const Write &) -> std::string { return "WRITE"; },
                   [](const Load &load) -> std::string {
                       return "LOAD " + to_string(load.address);
                   },
                   [](const Store &store) -> std::string {
                       return "STORE " + to_string(store.address);
                   },
                   [](const Add &add) -> std::string {
                       return "ADD " + to_string(add.address);
                   },
                   [](const Sub &sub) -> std::string {
                       return "SUB " + to_string(sub.address);
                   },
                   [](const Get &get) -> std::string {
                       return "GET " + to_string(get.address);
                   },
                   [](const Put &put) -> std::string {
                       return "PUT " + to_string(put.address);
                   },
                   [](const Rst &rst) -> std::string {
                       return "RST " + to_string(rst.address);
                   },
                   [](const Inc &inc) -> std::string {
                       return "INC " + to_string(inc.address);
                   },
                   [](const Dec &dec) -> std::string {
                       return "DEC " + to_string(dec.address);
                   },
                   [](const Shl &shl) -> std::string {
                       return "SHL " + to_string(shl.address);
                   },
                   [](const Shr &shr) -> std::string {
                       return "SHR " + to_string(shr.address);
                   },
                   [](const Jump &jump) -> std::string {
                       return "JUMP " + std::to_string(jump.line);
                   },
                   [](const Jpos &jpos) -> std::string {
                       return "JPOS " + std::to_string(jpos.line);
                   },
                   [](const Jzero &jzero) -> std::string {
                       return "JZERO " + std::to_string(jzero.line);
                   },
                   [](const Strk &strk) -> std::string {
                       return "STRK " + to_string(strk.reg);
                   },
                   [](const Jumpr &jumpr) -> std::string {
                       return "JUMPR " + to_string(jumpr.reg);
                   },
                   [](const Halt &) -> std::string { return "HALT"; },
                   [](const Comment &comment) -> std::string {
                       std::string str = "#";
                       for (auto i = 0u; i < comment.indent; i++)
                           str += "=";
                       return str + " " + comment.comment;
                   }},
        instruction);
}

inline auto mnemonic_from_string(const std::string &instruction)
    -> std::optional<Instruction> {
    if (instruction == "READ")
        return Read{};
    if (instruction == "WRITE")
        return Write{};
    if (instruction == "LOAD")
        return Load{};
    if (instruction == "STORE")
        return Store{};
    if (instruction == "ADD")
        return Add{};
    if (instruction == "SUB")
        return Sub{};
    if (instruction == "GET")
        return Get{};
    if (instruction == "PUT")
        return Put{};
    if (instruction == "RST")
        return Rst{};
    if (instruction == "INC")
        return Inc{};
    if (instruction == "DEC")
        return Dec{};
    if (instruction == "SHL")
        return Shl{};
    if (instruction == "SHR")
        return Shr{};
    if (instruction == "JUMP")
        return Jump{};
    if (instruction == "JPOS")
        return Jpos{};
    if (instruction == "JZERO")
        return Jzero{};
    if (instruction == "STRK")
        return Strk{};
    if (instruction == "JUMPR")
        return Jumpr{};
    if (instruction == "HALT")
        return Halt{};

    return std::nullopt;
}

inline void set_jump_location(Instruction &instruction, uint64_t location) {
    std::visit(overloaded{[&](Jump &jump) { jump.line = location; },
                          [&](Jpos &jpos) { jpos.line = location; },
                          [&](Jzero &jzero) { jzero.line = location; },
                          [&](auto arg) { assert(false); }},
               instruction);
}

inline void set_instruction_register(Instruction &instruction, Register reg) {
    std::visit(overloaded{[&](Load &load) { load.address = reg; },
                          [&](Store &store) { store.address = reg; },
                          [&](Add &add) { add.address = reg; },
                          [&](Sub &sub) { sub.address = reg; },
                          [&](Get &get) { get.address = reg; },
                          [&](Put &put) { put.address = reg; },
                          [&](Rst &rst) { rst.address = reg; },
                          [&](Inc &inc) { inc.address = reg; },
                          [&](Dec &dec) { dec.address = reg; },
                          [&](Shl &shl) { shl.address = reg; },
                          [&](Shr &shr) { shr.address = reg; },
                          [&](Strk &strk) { strk.reg = reg; },
                          [&](Jumpr &jumpr) { jumpr.reg = reg; },
                          [&](auto arg) { assert(false); }},
               instruction);
}

struct Line {
    Instruction instruction;
    std::string comment{};
};

} // namespace instruction
