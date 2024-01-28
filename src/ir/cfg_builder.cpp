#include "cfg_builder.hpp"
#include "instruction.hpp"

/*
 *
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

*/
namespace lir {

void CfgBuilder::push_current_block() {
    cfg.basic_blocks.push_back(current_block);
    current_block = Block{};
}

auto CfgBuilder::build() -> Cfg {
    for (const auto &instruction : instructions) {
        std::visit(overloaded{
                       [&](const Jump &) {
                           current_block.instructions.push_back(instruction);
                           push_current_block();
                       },
                       [&](const Jpos &) {
                           current_block.instructions.push_back(instruction);
                           push_current_block();
                       },
                       [&](const Jzero &) {
                           current_block.instructions.push_back(instruction);
                           push_current_block();
                       },
                       [&](const Jumpr &) {
                           current_block.instructions.push_back(instruction);
                           push_current_block();
                       },
                       [&](const Halt &) {
                           current_block.instructions.push_back(instruction);
                           push_current_block();
                       },
                       [&](const Label &) {
                           push_current_block();
                           current_block.instructions.push_back(instruction);
                       },
                       [&](const auto &) { current_block.instructions.push_back(instruction); },
                   },
                   instruction);
    }
    return cfg;
}
} // namespace lir
