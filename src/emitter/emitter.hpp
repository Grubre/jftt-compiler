#pragma once
#include "ast.hpp"
#include "error.hpp"
#include "expected.hpp"
#include "instruction.hpp"
#include <algorithm>
#include <stack>
#include <unordered_map>

namespace emitter {

struct Variable {
    std::string source;
    std::string name;

    bool operator==(const Variable &other) const {
        return source == other.source && name == other.name;
    }
};

} // namespace emitter

namespace std {
template <> struct hash<emitter::Variable> {
    size_t operator()(const emitter::Variable &v) const {
        // Use std::hash for std::string and combine the hashes
        return hash<string>()(v.source) ^ (hash<string>()(v.name) << 1);
    }
};
} // namespace std

namespace emitter {

constexpr auto indent_level_main = 0;
constexpr auto indent_level_middle = 4;
constexpr auto indent_level_sub = 8;

struct MemoryLocation {
    uint64_t address;
    uint64_t size;
    bool is_pointer = false;
};

struct Procedure {
    uint64_t entrypoint;
    uint64_t memory_loc;
    const ast::Procedure *procedure;
};

// REGISTER A - Accumulator
// REGISTER B - MAR

using Location = MemoryLocation;

struct Jumps {
    std::vector<uint64_t> jumps_if_false;
    std::vector<uint64_t> jumps_if_true;
};

class Emitter {
  public:
    Emitter() = delete;
    Emitter(ast::Program &&program) : program(std::move(program)) {
        // The first jump jumps to the main procedure but we don't know where
        // that is yet so we just put a placeholder address here (0)
        lines.push_back(
            instruction::Line{instruction::Jump{0}, "Jump to main"});

        registers.push(instruction::Register::C);
        registers.push(instruction::Register::D);
        registers.push(instruction::Register::E);
        registers.push(instruction::Register::F);
        registers.push(instruction::Register::G);
        registers.push(instruction::Register::H);
    };

    void emit();
    void emit_procedure(const ast::Procedure &procedure);

    void emit_comment(const instruction::Comment &comment);
    auto emit_condition(const ast::Condition &condition,
                        const std::string &comment_when_false) -> Jumps;

    void emit_command(const ast::Command &command);
    void emit_read(const ast::Identifier &identifier);
    void emit_write(const ast::Value &value);
    void emit_assignment(const ast::Assignment &assignment);
    void emit_if(const ast::If &if_statement);
    void emit_repeat(const ast::Repeat &repeat);
    void emit_while(const ast::While &while_statement);
    void emit_call(const ast::Call &call);

    auto get_variable(const Token &variable) -> Location *;

    void assign_memory(const std::vector<ast::Declaration> &declarations);

    void backup_register(instruction::Register reg);
    void set_register(instruction::Register reg, uint64_t value);
    void set_register(instruction::Register reg, const ast::Value &value);
    void set_accumulator(const ast::Value &value);
    void set_accumulator(uint64_t value);
    void set_mar(uint64_t value);
    void set_mar(const ast::Identifier &identifier);
    void handle_pointer(const ast::Identifier &identifier);
    void set_memory(uint64_t value);
    void set_memory(const ast::Identifier &identifier);
    void set_jump_location(instruction::Instruction &instruction,
                           uint64_t location);

    void emit_line(const instruction::Instruction &instruction);
    void emit_line_with_comment(const instruction::Instruction &instruction,
                                const instruction::Comment &comment);
    void push_comment(const instruction::Comment &comment);
    void push_error(const std::string &message, unsigned line, unsigned column);

    bool is_pointer(const Token &variable);

    auto get_lines() const -> const std::vector<instruction::Line> & {
        return lines;
    }
    auto get_errors() const -> const std::vector<Error> & { return errors; }

  private:
    ast::Program program;
    std::unordered_map<std::string, Procedure> procedures{};
    std::vector<instruction::Line> lines{};
    std::vector<Error> errors{};

    std::deque<instruction::Comment> comments{};

    std::string current_source = "";

    uint64_t stack_pointer = 0;
    std::stack<instruction::Register> registers{};
    std::unordered_map<Variable, Location> variables{};
};

} // namespace emitter
// def multiply_logarithmically(a, b):
//     result = 0
//
//     while b > 0:
//         if b % 2 == 1:
//             result += a
//         a *= 2
//         b //= 2
//
//     return result
