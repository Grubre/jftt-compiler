#pragma once
#include "low_level_ir.hpp"
#include <set>

namespace lir {

struct Block {
    uint64_t id;
    std::set<uint64_t> live_in{};
    std::set<uint64_t> live_out{};
    std::vector<VirtualInstruction> instructions{};
    std::vector<uint64_t> previous_blocks_ids{};
    std::vector<uint64_t> next_blocks_ids{};

    auto to_string() const -> std::string {
        auto result = std::format("Block {}:\n", id);
        result += "live in: ";
        for (const auto &live_in : live_in) {
            result += std::to_string(live_in) + ",";
        }
        result += "\nlive out: ";
        for (const auto &live_out : live_out) {
            result += std::to_string(live_out) + ",";
        }
        result += "\n";
        for (const auto &instr : instructions) {
            result += "\t" + lir::to_string(instr) + "\n";
        }

        return result;
    }
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
    void calculate_live_ins_and_outs();

  private:
    uint64_t current_block_id = 0;
    Block current_block{.id = current_block_id};
    Cfg cfg{};
    const std::vector<VirtualInstruction> &instructions;

    std::unordered_map<std::string, uint64_t> label_to_block_id{};
};

} // namespace lir
