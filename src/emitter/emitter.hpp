#pragma once
#include "expected.hpp"
#include "instruction.hpp"
#include "parser.hpp"
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
    const parser::Procedure *procedure;
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
    Emitter(parser::Program &&program) : program(std::move(program)) {
        // The first jump jumps to the main procedure but we don't know where
        // that is yet so we just put a placeholder address here (0)
        lines.push_back(Line{Jump{0}, "Jump to main"});

        registers.push(Register::C);
        registers.push(Register::D);
        registers.push(Register::E);
        registers.push(Register::F);
        registers.push(Register::G);
        registers.push(Register::H);
    };

    void emit();
    void emit_procedure(const parser::Procedure &procedure);

    void emit_comment(const Comment &comment);
    auto emit_condition(const parser::Condition &condition,
                        const std::string &comment_when_false) -> Jumps;

    void emit_command(const parser::Command &command);
    void emit_read(const parser::Identifier &identifier);
    void emit_write(const parser::Value &value);
    void emit_assignment(const parser::Assignment &assignment);
    void emit_if(const parser::If &if_statement);
    void emit_repeat(const parser::Repeat &repeat);
    void emit_while(const parser::While &while_statement);
    void emit_call(const parser::Call &call);

    auto get_variable(const std::string &name) -> Location &;

    void assign_memory(const std::vector<parser::Declaration> &declarations);

    void backup_register(Register reg);
    void set_register(Register reg, uint64_t value);
    void set_register(Register reg, const parser::Value &value);
    void set_accumulator(const parser::Value &value);
    void set_accumulator(uint64_t value);
    void set_mar(uint64_t value);
    void set_mar(const parser::Identifier &identifier);
    void handle_pointer(const parser::Identifier &identifier);
    void set_memory(uint64_t value);
    void set_memory(const parser::Identifier &identifier);
    void set_jump_location(Instruction &instruction, uint64_t location);

    void emit_line(const Instruction &instruction);
    void emit_line_with_comment(const Instruction &instruction,
                                const Comment &comment);
    void push_comment(const Comment &comment);
    void push_error(const std::string &message, unsigned line, unsigned column);

    bool is_pointer(const std::string &name);

    auto get_lines() const -> const std::vector<Line> & { return lines; }

  private:
    parser::Program program;
    std::unordered_map<std::string, Procedure> procedures{};
    std::vector<Line> lines{};
    std::vector<Error> errors{};

    std::deque<Comment> comments{};

    std::string current_source = "";

    uint64_t stack_pointer = 0;
    std::stack<Register> registers{};
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
