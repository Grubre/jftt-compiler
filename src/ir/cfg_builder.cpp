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
    split_into_blocks();
    connect_blocks();
    return cfg;
}

void CfgBuilder::connect_blocks() {
    for (auto i = 0u; i < cfg.basic_blocks.size(); i++) {
        const auto &block = cfg.basic_blocks[i];
        const auto &last_instruction = block.instructions.back();
        std::visit(overloaded{
                       [&](const Jump &jump) {
                           const auto label = jump.label;
                           const auto block_with_label_id = label_to_block_id[label];
                           cfg.basic_blocks[i].next.push_back(block_with_label_id);
                           cfg.basic_blocks[block_with_label_id].prev.push_back(i);
                       },
                       [&](const Jpos &jpos) {
                           const auto label = jpos.label;
                           const auto block_with_label_id = label_to_block_id[label];
                           cfg.basic_blocks[i].next.push_back(block_with_label_id);
                           if (i < cfg.basic_blocks.size() - 1)
                               cfg.basic_blocks[i].next.push_back(i + 1);
                           cfg.basic_blocks[block_with_label_id].prev.push_back(i);
                       },
                       [&](const Jzero &jzero) {
                           const auto label = jzero.label;
                           const auto block_with_label_id = label_to_block_id[label];
                           cfg.basic_blocks[i].next.push_back(block_with_label_id);
                           if (i < cfg.basic_blocks.size() - 1)
                               cfg.basic_blocks[i].next.push_back(i + 1);
                           cfg.basic_blocks[block_with_label_id].prev.push_back(i);
                       },
                       [&](const Jumpr &jumpr) {
                           // TODO: what put here?
                           // probably there is no block since the procedure just returns
                       },
                       [&](const Halt &) {},
                       [&](const auto &) {},
                   },
                   last_instruction);
    }
}

void CfgBuilder::split_into_blocks() {
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
                       [&](const Label &label) {
                           label_to_block_id[label.name] = cfg.basic_blocks.size();
                           push_current_block();
                           current_block.instructions.push_back(instruction);
                       },
                       [&](const auto &) { current_block.instructions.push_back(instruction); },
                   },
                   instruction);
    }
}
} // namespace lir
