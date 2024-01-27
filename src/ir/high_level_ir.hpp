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

struct Read {
    Variable loc;
};
struct Write {
    Operand op;
};
struct EmitConst {
    Variable loc;
    uint64_t constant;
};
struct BinaryOp {
    Variable loc;
    Operand op1;
    char operator_;
    Operand op2;
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
    std::variant<Read, Write, EmitConst, BinaryOp, Jmp, JmpIf, Halt, Store, Load, BeginProcedure>;

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

    auto get_ir() const -> HighLevelIR const & { return ir; }
    auto get_errors() const -> std::vector<Error> const & { return errors; }

    auto get_variable_signature(const ast::Identifier &identifier) const -> std::string;
    auto get_variable_signature(const Token &identifier) const -> std::string;

    auto get_variable_declaration(const Token &pidentifier) const -> VariableDeclaration const * {
        const auto declaration = &variables.at(get_variable_signature(pidentifier));
        return declaration;
    }

    auto get_variable(const ast::Identifier &identifier) const -> Variable {
        const auto declaration = get_variable_declaration(identifier.name);
        const auto offset = identifier.index ? std::stoull(identifier.index->lexeme) : 0;
        return Variable{.id = declaration->id, .offset = offset};
    }

    auto get_next_variable_id() -> uint64_t { return next_variable_id++; }

  private:
    std::string current_source = "";
    std::unordered_map<VariableSignature, VariableDeclaration> variables;
    uint64_t next_variable_id = 0;
    uint64_t next_label_id = 0;
    HighLevelIR ir;
    std::vector<Error> errors;
};
} // namespace hir
