#pragma once
#include "ast.hpp"
#include "cfg_builder.hpp"
#include "low_level_ir.hpp"

namespace lir {
struct ResolvedVariable {
    uint64_t vregister_id;
    bool is_pointer;
};

struct Procedure {
    std::vector<VirtualRegister> args;
};

struct RegisterInterferenceGraph {
    std::vector<std::set<uint64_t>> neighbours;
};

class LirEmitter {
  public:
    using Instructions = std::vector<VirtualInstruction>;
    using ProcedureCodes = std::unordered_map<std::string, Instructions>;
    LirEmitter() = delete;
    LirEmitter(ast::Program &&program) : program(std::move(program)) {}

    void emit();
    void emit_procedure(const ast::Procedure &procedure);
    void emit_context(const ast::Context &context);
    void emit_commands(const std::span<const ast::Command> commands);
    void emit_label(const std::string &label);

    void emit_condition(const ast::Condition &condition, const std::string &true_label, const std::string &false_label);
    void emit_read(const ast::Read &read);
    void emit_write(const ast::Write &write);
    void emit_if(const ast::If &if_statement);
    void emit_repeat(const ast::Repeat &repeat);
    void emit_assignment(const ast::Assignment &assignment);
    void emit_while(const ast::While &while_statement);
    void emit_call(const ast::Call &call);

    void emit_constant(VirtualRegister vregister, const ast::Num &num);

    void push_instruction(VirtualInstruction instruction);

    auto get_variable(const ast::Identifier &identifier) -> ResolvedVariable;
    auto get_variable(const Token &identifier) -> ResolvedVariable;
    auto new_vregister() -> VirtualRegister;
    auto get_label_str(const std::string &label) -> std::string;
    void put_to_vreg_or_mem(const ast::Identifier &identifier);
    auto get_from_vreg_or_load_from_mem(const ast::Identifier &identifier) -> VirtualRegister;
    auto get_from_vreg_or_load_from_mem(const Token &identifier) -> VirtualRegister;

    void set_vreg(const ast::Value &value, VirtualRegister vreg);
    auto put_constant_to_vreg_or_get(const ast::Value &value) -> VirtualRegister;

    auto get_procedure_codes() -> ProcedureCodes;
    auto get_flattened_instructions() -> Instructions;

    void allocate_registers(Cfg *cfg);

  private:
    ast::Program program;

    std::unordered_map<std::string, Procedure> procedures;
    std::unordered_map<std::string, ResolvedVariable> resolved_variables{};
    ProcedureCodes instructions;

    VirtualRegister next_vregister_id = 1;
    uint64_t next_label_id = 0;
    std::string current_source = "";

    Cfg *cfg;
    RegisterInterferenceGraph interference_graph;

    static constexpr auto main_label = "MAIN";
    static constexpr VirtualRegister regA = 0;
};
} // namespace lir
