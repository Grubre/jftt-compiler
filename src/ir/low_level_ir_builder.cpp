#include "low_level_ir_builder.hpp"
#include "common.hpp"

namespace lir {

void LirEmitter::emit() {
    for (const auto &procedure : program.procedures) {
        emit_procedure(procedure);
    }

    push_instruction(Label{main_label});
    emit_context(program.main);
    push_instruction(Halt{});
}

void LirEmitter::emit_procedure(const ast::Procedure &procedure) {
    current_source = procedure.name.lexeme;

    const auto return_address_vreg = new_vregister();
    procedures[current_source] = Procedure{{return_address_vreg}};

    for (const auto &variable : procedure.args) {
        const auto vreg = new_vregister();
        resolved_variables[current_source + "@" + variable.identifier.lexeme] = ResolvedVariable{vreg, true};
        procedures[current_source].args.push_back(vreg);
    }

    push_instruction(Label{current_source});
    emit_context(procedure.context);
    push_instruction(Jumpr{return_address_vreg});
}

void LirEmitter::emit_context(const ast::Context &context) {
    for (const auto &variable : context.declarations) {
        resolved_variables[current_source + "@" + variable.identifier.lexeme] =
            ResolvedVariable{new_vregister(), false};
    }
    emit_commands(context.commands);
}

void LirEmitter::emit_commands(const std::span<const ast::Command> commands) {
    for (const auto &command : commands) {
        std::visit(overloaded{[&](const ast::Read &read) { emit_read(read); },
                              [&](const ast::Write &write) { emit_write(write); },
                              [&](const ast::If &if_statement) { emit_if(if_statement); },
                              [&](const ast::Repeat &repeat) { emit_repeat(repeat); },
                              [&](const ast::Assignment &assignment) { emit_assignment(assignment); },
                              [&](const ast::While &while_statement) { emit_while(while_statement); },
                              [&](const ast::Call &call) { emit_call(call); },
                              [&](const ast::InlinedProcedure &procedure) { emit_commands(procedure.commands); }},
                   command);
    }
}

void LirEmitter::emit_call(const ast::Call &call) {
    const auto return_address_vreg = procedures[call.name.lexeme].args[0];

    for (auto i = 0u; i < call.args.size(); ++i) {
        const auto arg = call.args[i];
        const auto vreg = procedures[call.name.lexeme].args[i + 1];
        // FIXME: Here it is supposed to pass the address, not the value
        get_from_vreg_or_load_from_mem(arg);
        push_instruction(Put{vreg});
    }

    push_instruction(Strk{return_address_vreg});
    push_instruction(Jump{call.name.lexeme});
}

void LirEmitter::emit_read(const ast::Read &read) {
    // NOTE: Potentially need to change the order of operations because A might be polluted by setting the memory
    push_instruction(Read{});
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
    push_instruction(Write{});
}

void LirEmitter::emit_condition(const ast::Condition &condition, const std::string &true_label,
                                const std::string &false_label) {
    const auto lhs_minus_rhs = [&]() {
        const auto rhs_vreg = put_constant_to_vreg_or_get(condition.rhs);
        set_vreg(condition.lhs, regA);
        push_instruction(Sub{});
    };
    const auto rhs_minus_lhs = [&]() {
        const auto rhs_vreg = put_constant_to_vreg_or_get(condition.lhs);
        set_vreg(condition.rhs, regA);
        push_instruction(Sub{});
    };

    // NOTE: Doesnt work for pointers
    switch (condition.op.token_type) {
    case TokenType::LessEquals:
        lhs_minus_rhs();
        push_instruction(Jpos{false_label});
        break;
    case TokenType::GreaterEquals:
        rhs_minus_lhs();
        push_instruction(Jpos{false_label});
        break;
    default:
        assert(false && "Not supported yet");
    }
}

void LirEmitter::emit_if(const ast::If &if_statement) {
    const auto true_label = get_label_str("IF_TRUE");
    const auto false_label = get_label_str("IF_FALSE");
    const auto endif_label = get_label_str("ENDIF");

    emit_condition(if_statement.condition, true_label, false_label);

    emit_commands(if_statement.commands);
    push_instruction(Jump{endif_label});

    if (if_statement.else_commands) {
        emit_label(false_label);
        emit_commands(*if_statement.else_commands);
    }
    emit_label(endif_label);
}

void LirEmitter::emit_while(const ast::While &while_statement) {
    const auto true_label = get_label_str("while");
    const auto false_label = get_label_str("endwhile");

    emit_label(true_label);
    emit_condition(while_statement.condition, true_label, false_label);
    emit_commands(while_statement.commands);
    push_instruction(Jump{true_label});
    emit_label(false_label);
}

void LirEmitter::emit_repeat(const ast::Repeat &repeat) {
    const auto true_label = get_label_str("repeat");
    const auto false_label = get_label_str("endrepeat");

    emit_label(true_label);
    emit_commands(repeat.commands);
    emit_condition(repeat.condition, true_label, false_label);
    emit_label(false_label);
}

void LirEmitter::emit_assignment(const ast::Assignment &assignment) {
    const auto identifier = assignment.identifier;

    if (std::holds_alternative<ast::Value>(assignment.expression)) {
        const auto value = std::get<ast::Value>(assignment.expression);
        const auto vreg = put_constant_to_vreg_or_get(value);
        set_vreg(identifier, vreg);
        return;
    }

    const auto binary_expression = std::get<ast::BinaryExpression>(assignment.expression);
    const auto assignee = get_variable(identifier).vregister_id;
    const auto lhs = put_constant_to_vreg_or_get(binary_expression.lhs);
    const auto rhs = put_constant_to_vreg_or_get(binary_expression.rhs);

    switch (binary_expression.op.token_type) {
    case TokenType::Plus:
        push_instruction(Get{lhs});
        push_instruction(Add{rhs});
        push_instruction(Put{assignee});
        break;
    case TokenType::Minus:
        push_instruction(Get{lhs});
        push_instruction(Sub{rhs});
        push_instruction(Put{assignee});
        break;
    default:
        assert(false && "Not supported yet");
    }
}

void LirEmitter::push_instruction(VirtualInstruction instruction) {
    instructions[current_source].push_back(instruction);
}

void LirEmitter::put_to_vreg_or_mem(const ast::Identifier &identifier) {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Put{variable.vregister_id});
    } else {
        push_instruction(Store{variable.vregister_id});
    }
}

void LirEmitter::get_from_vreg_or_load_from_mem(const ast::Identifier &identifier) {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Get{variable.vregister_id});
    } else {
        push_instruction(Load{variable.vregister_id});
    }
}

void LirEmitter::get_from_vreg_or_load_from_mem(const Token &identifier) {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Get{variable.vregister_id});
    } else {
        push_instruction(Load{variable.vregister_id});
    }
}

auto LirEmitter::get_variable(const ast::Identifier &identifier) -> ResolvedVariable {
    const auto signature = current_source + "@" + identifier.name.lexeme;
    return resolved_variables.at(signature);
}

auto LirEmitter::get_variable(const Token &identifier) -> ResolvedVariable {
    const auto signature = current_source + "@" + identifier.lexeme;
    return resolved_variables.at(signature);
}

auto LirEmitter::new_vregister() -> VirtualRegister { return VirtualRegister{next_vregister_id++}; }

void LirEmitter::set_vreg(const ast::Value &value, VirtualRegister vreg) {
    if (std::holds_alternative<ast::Num>(value)) {
        const auto num = std::get<ast::Num>(value);
        emit_constant(vreg, num);
    } else {
        const auto identifier = std::get<ast::Identifier>(value);
        get_from_vreg_or_load_from_mem(identifier);
    }
}

void LirEmitter::emit_constant(VirtualRegister vregister, const ast::Num &num) {
    const auto num_value = std::stoull(num.lexeme);
    push_instruction(Rst{vregister});
    for (auto i = 0u; i < num_value; i++) {
        push_instruction(Inc{vregister});
    }
}

void LirEmitter::emit_label(const std::string &label) { push_instruction(Label{label}); }

auto LirEmitter::put_constant_to_vreg_or_get(const ast::Value &value) -> VirtualRegister {
    if (std::holds_alternative<ast::Num>(value)) {
        const auto num = std::get<ast::Num>(value);
        const auto vreg = new_vregister();
        emit_constant(vreg, num);
        return vreg;
    }
    return get_variable(std::get<ast::Identifier>(value)).vregister_id;
}

auto LirEmitter::get_label_str(const std::string &label) -> std::string {
    return std::format("{}#{}", label, next_vregister_id++);
}

auto LirEmitter::get_procedure_codes() -> ProcedureCodes { return instructions; }
auto LirEmitter::get_flattened_instructions() -> Instructions {
    auto result = Instructions{};
    for (const auto &[_, instructions] : instructions) {
        result.insert(result.end(), instructions.begin(), instructions.end());
    }
    return result;
}
} // namespace lir
