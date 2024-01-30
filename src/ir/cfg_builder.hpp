#pragma once
#include "low_level_ir.hpp"

namespace lir {

struct Block {
    uint64_t id;
    std::vector<uint64_t> live_in;
    std::vector<uint64_t> live_out;
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
    CfgBuilder(const std::vector<VirtualInstruction> &instructions) : instructions(instructions) {}

    auto build() -> Cfg;

  private:
    void split_into_blocks();
    void connect_blocks();
    void push_current_block();

  private:
    uint64_t current_block_id = 0;
    Block current_block{.id = current_block_id};
    Cfg cfg{};
    const std::vector<VirtualInstruction> &instructions;

    std::unordered_map<std::string, uint64_t> label_to_block_id{};
};

} // namespace lir
