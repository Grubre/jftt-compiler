#pragma once
#include "expected.hpp"
#include "instruction.hpp"
#include "parser.hpp"
#include <algorithm>
#include <stack>
#include <unordered_map>

namespace emitter {

struct MemoryLocation {
    uint64_t address;
    uint64_t size;
};

// REGISTER A - Accumulator
// REGISTER B - MAR

using Location = MemoryLocation;

class Emitter {
  public:
    Emitter() = delete;
    Emitter(parser::Program &&program) : program(std::move(program)) {
        // The first jump jumps to the main procedure but we don't know where
        // that is yet so we just put a placeholder address here (0)
        // lines.push_back(Line{Jump{0}, "Jump to main"});

        registers.push(Register::C);
        registers.push(Register::D);
        registers.push(Register::E);
        registers.push(Register::F);
        registers.push(Register::G);
        registers.push(Register::H);
    };

    void emit();
    void emit_procedure(const parser::Procedure &procedure);

    void emit_read(const parser::Identifier &identifier);
    void emit_write(const parser::Value &value);

    void assign_var_memory();

    void backup_register(Register reg);
    void set_register(Register reg, uint64_t value);
    void set_mar(const parser::Identifier &identifier);
    void set_memory(const parser::Identifier &identifier);

    auto get_lines() const -> const std::vector<Line> & { return lines; }

  private:
    std::unordered_map<std::string, Location> variables{};
    parser::Program program;
    std::unordered_map<Token, uint64_t> procedure_entrypoints{};
    std::vector<Line> lines{};
    std::vector<Error> errors{};

    uint64_t stack_pointer = 0;
    std::stack<Register> registers{};
};

} // namespace emitter
