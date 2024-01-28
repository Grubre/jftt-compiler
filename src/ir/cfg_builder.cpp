#include "cfg_builder.hpp"
#include "common.hpp"
#include "instruction.hpp"

namespace lir {

auto generate_dot(const Cfg &cfg) -> std::string {
    std::string dot = "digraph cfg {\n";
    dot += "    node [shape=record];\n";
    dot += "    edge [arrowhead=normal];\n";

    for (const auto &block : cfg.basic_blocks) {
        dot += "    block" + std::to_string(block.id) + " [label=\"Block " + std::to_string(block.id) + "|";

        for (const auto &instr : block.instructions) {
            dot += to_string(instr) + "\\l"; // '\\l' denotes left-align text in Graphviz
        }

        dot += "\"];\n";
    }

    for (const auto &block : cfg.basic_blocks) {
        for (auto next_id : block.next_blocks_ids) {
            dot += "    block" + std::to_string(block.id) + " -> block" + std::to_string(next_id) + ";\n";
        }
    }

    dot += "}\n";
    return dot;
}

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
                           cfg.basic_blocks[i].next_blocks_ids.push_back(block_with_label_id);
                           cfg.basic_blocks[block_with_label_id].previous_blocks_ids.push_back(i);
                       },
                       [&](const Jpos &jpos) {
                           const auto label = jpos.label;
                           const auto block_with_label_id = label_to_block_id[label];
                           cfg.basic_blocks[i].next_blocks_ids.push_back(block_with_label_id);
                           if (i < cfg.basic_blocks.size() - 1)
                               cfg.basic_blocks[i].next_blocks_ids.push_back(i + 1);
                           cfg.basic_blocks[block_with_label_id].previous_blocks_ids.push_back(i);
                       },
                       [&](const Jzero &jzero) {
                           const auto label = jzero.label;
                           const auto block_with_label_id = label_to_block_id[label];
                           cfg.basic_blocks[i].next_blocks_ids.push_back(block_with_label_id);
                           if (i < cfg.basic_blocks.size() - 1)
                               cfg.basic_blocks[i].next_blocks_ids.push_back(i + 1);
                           cfg.basic_blocks[block_with_label_id].previous_blocks_ids.push_back(i);
                       },
                       [&](const Jumpr &jumpr) {
                           // TODO: what put here?
                           // NOTE: Probably all blocks that call this function
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
