#pragma once
#include "ast.hpp"
#include "error.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace hir {
struct Variable {
    uint64_t id;
    uint64_t offset;
};

struct VariableDeclaration {
    uint64_t id;
    bool is_pointer;
};

using Operand = std::variant<Variable, uint64_t>;

enum class BinaryOperator { Add, Sub, Shl, Shr };

struct Read {
    Variable loc;
};
struct Write {
    Operand op;
};
struct Assign {
    Variable loc;
    Operand op;
};
struct BinaryOp {
    Variable loc;
    Operand lhs;
    BinaryOperator operator_;
    Operand rhs;
};
struct Jmp {
    std::string label;
};
struct JmpIf {
    std::string label;
    ast::Condition cond;
};
struct Store {
    uint64_t memory_loc;
    Variable op;
};
struct Load {
    Variable loc;
    uint64_t memory_loc;
};
struct BeginProcedure {
    std::string name;
};
struct Halt {};

using HighLevelIRInstruction =
    std::variant<Read, Write, Assign, BinaryOp, Jmp, JmpIf, Halt, Store, Load, BeginProcedure>;

struct HighLevelIR {
    std::vector<HighLevelIRInstruction> instructions;
    std::unordered_map<std::string, uint64_t> labels;
    std::unordered_map<uint64_t, uint64_t> constant_frequencies;
    std::unordered_map<std::string, uint64_t> function_call_frequencies;
};

using VariableSignature = std::string;

class AstToHir {
  public:
    AstToHir() = delete;
    AstToHir(const ast::Program &program);

    void push_instruction(HighLevelIRInstruction instruction, std::optional<std::string> label = std::nullopt);
    void emit_procedure(const ast::Procedure &procedure);
    void emit_context(const ast::Context &context);
    void emit_commands(const std::span<const ast::Command> &commands);

    void emit_write(const ast::Write &write);
    void emit_assignment(const ast::Assignment &assignment);

    auto get_ir() const -> HighLevelIR const & { return ir; }
    auto get_errors() const -> std::vector<Error> const & { return errors; }

    auto get_variable_signature(const ast::Identifier &identifier) const -> std::string;
    auto get_variable_signature(const Token &identifier) const -> std::string;

    auto get_variable_declaration(const Token &pidentifier) const -> VariableDeclaration const *;
    auto get_variable(const ast::Identifier &identifier) const -> Variable;
    auto get_constant(const ast::Num &num) -> uint64_t;
    auto get_operand(const ast::Value &value) -> Operand;
    auto get_next_variable_id() -> uint64_t;

  private:
    std::string current_source = "";
    std::unordered_map<VariableSignature, VariableDeclaration> variables;
    uint64_t next_variable_id = 0;
    uint64_t next_label_id = 0;
    HighLevelIR ir;
    std::vector<Error> errors;
};
} // namespace hir
