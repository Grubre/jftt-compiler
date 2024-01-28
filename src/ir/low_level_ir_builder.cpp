#include "low_level_ir_builder.hpp"

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace lir {

void LirEmitter::emit() {
    for (const auto &procedure : program.procedures) {
        emit_procedure(procedure);
    }
}

void LirEmitter::emit_procedure(const ast::Procedure &procedure) {
    current_source = procedure.signature();
    emit_context(procedure.context);
}

void LirEmitter::emit_context(const ast::Context &context) {
    for (const auto &command : context.commands) {
        std::visit(overloaded{[&](const ast::Read &read) { emit_read(read); },
                              [&](const ast::Write &write) { emit_write(write); },
                              [&](const ast::If &if_statement) { emit_if(if_statement); },
                              [&](const ast::Repeat &repeat) { emit_repeat(repeat); },
                              [&](const ast::Assignment &assignment) { emit_assignment(assignment); },
                              [&](const ast::While &while_statement) { emit_while(while_statement); },
                              [&](const ast::Call &call) { emit_call(call); }},
                   command);
    }
}

void LirEmitter::emit_read(const ast::Read &read) {
    // NOTE: Potentially need to change the order of operations because A might be polluted by setting the memory
    instructions.push_back(Read{});
    put_to_vreg_or_mem(read.identifier);
}

void LirEmitter::emit_write(const ast::Write &write) {
    if (std::holds_alternative<ast::Num>(write.value)) {
        const auto num = std::get<ast::Num>(write.value);
        emit_constant(regA, num);
    } else {
        const auto identifier = std::get<ast::Identifier>(write.value);
        get_from_vreg_or_load_from_mem(identifier);
    }
    instructions.push_back(Write{});
}

void LirEmitter::emit_condition(const ast::Condition &condition, const std::string &true_label,
                                const std::string &false_label) {}

void LirEmitter::emit_if(const ast::If &if_statement) {}

void LirEmitter::put_to_vreg_or_mem(const ast::Identifier &identifier) {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        instructions.push_back(Put{variable.vregister_id});
    } else {
        instructions.push_back(Store{variable.vregister_id});
    }
}

void LirEmitter::get_from_vreg_or_load_from_mem(const ast::Identifier &identifier) {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        instructions.push_back(Get{variable.vregister_id});
    } else {
        instructions.push_back(Load{variable.vregister_id});
    }
}

auto LirEmitter::get_variable(const ast::Identifier &identifier) -> ResolvedVariable {
    const auto signature = current_source + "@" + identifier.name.lexeme;
    return resolved_variables.at(signature);
}

auto LirEmitter::get_vregister() -> VirtualRegister { return VirtualRegister{next_vregister_id++}; }

auto LirEmitter::get_label_str(const std::string &label) -> std::string {
    return std::format("{}#{}", label, next_vregister_id++);
}

} // namespace lir
