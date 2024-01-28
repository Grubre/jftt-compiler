#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lir {
using VirtualRegister = uint64_t;

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

struct Block {
    uint64_t id;
    std::vector<VirtualInstruction> instructions;
    std::vector<uint64_t> next;
};
struct Cfg {
    std::vector<Block> basic_blocks;
    std::unordered_map<std::string, uint64_t> label;
};

} // namespace lir
