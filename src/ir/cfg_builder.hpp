#pragma once
#include "low_level_ir.hpp"

namespace lir {

struct Block {
    uint64_t id;
    std::vector<VirtualInstruction> instructions;
    std::vector<uint64_t> prev;
    std::vector<uint64_t> next;
};

struct Cfg {
    std::vector<Block> basic_blocks;
};

class CfgBuilder {
  public:
    CfgBuilder() = delete;
    CfgBuilder(std::vector<VirtualInstruction> &&instructions) : instructions(std::move(instructions)) {}

    auto build() -> Cfg;
    void push_current_block();

  private:
    Block current_block;
    Cfg cfg{};
    std::vector<VirtualInstruction> instructions;
};

} // namespace lir
