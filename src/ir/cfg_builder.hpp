#pragma once
#include "low_level_ir.hpp"

namespace lir {

struct Block {
    std::vector<uint64_t> live_in;
    std::vector<uint64_t> live_out;
    uint64_t id;
    std::vector<VirtualInstruction> instructions;
    std::vector<uint64_t> previous_blocks_ids;
    std::vector<uint64_t> next_blocks_ids;
};

struct Cfg {
    std::vector<Block> basic_blocks;
};

auto generate_dot(const Cfg &cfg) -> std::string;

class CfgBuilder {
  public:
    CfgBuilder() = delete;
    CfgBuilder(std::vector<VirtualInstruction> &&instructions) : instructions(std::move(instructions)) {}

    auto build() -> Cfg;

  private:
    void split_into_blocks();
    void connect_blocks();
    void push_current_block();

  private:
    Block current_block;
    Cfg cfg{};
    std::vector<VirtualInstruction> instructions;

    std::unordered_map<std::string, uint64_t> label_to_block_id{};
};

} // namespace lir
