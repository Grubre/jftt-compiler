#pragma once
#include "ast.hpp"
#include "low_level_ir.hpp"

namespace lir {
struct ResolvedVariable {
    uint64_t vregister_id;
    bool is_pointer;
};

class LirEmitter {
  public:
    LirEmitter() = delete;
    LirEmitter(ast::Program &&program) : program(std::move(program)) { emit(); }

    void emit();
    void emit_procedure(const ast::Procedure &procedure);
    void emit_context(const ast::Context &context);

    void emit_condition(const ast::Condition &condition, const std::string &true_label, const std::string &false_label);
    void emit_read(const ast::Read &read);
    void emit_write(const ast::Write &write);
    void emit_if(const ast::If &if_statement);
    void emit_repeat(const ast::Repeat &repeat);
    void emit_assignment(const ast::Assignment &assignment);
    void emit_while(const ast::While &while_statement);
    void emit_call(const ast::Call &call);

    void emit_constant(VirtualRegister vregister, const ast::Num &num);

    auto get_variable(const ast::Identifier &identifier) -> ResolvedVariable;
    auto get_vregister() -> VirtualRegister;
    auto get_label_str(const std::string &label) -> std::string;
    void put_to_vreg_or_mem(const ast::Identifier &identifier);
    void get_from_vreg_or_load_from_mem(const ast::Identifier &identifier);

  private:
    std::vector<VirtualInstruction> instructions;
    ast::Program program;
    static constexpr VirtualRegister regA = 0;
    VirtualRegister next_vregister_id = 1;

    std::unordered_map<std::string, ResolvedVariable> resolved_variables{};

    std::string current_source = "";
};
} // namespace lir
