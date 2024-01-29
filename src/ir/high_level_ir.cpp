#include "high_level_ir.hpp"
#include "common.hpp"
template <class> inline constexpr bool always_false_v = false;

using namespace hir;
AstToHir::AstToHir(const ast::Program &program) {
    push_instruction(Jmp{"MAIN"});

    for (const auto &procedure : program.procedures) {
        emit_procedure(procedure);
    }

    push_instruction(Halt{});
}

void AstToHir::emit_procedure(const ast::Procedure &procedure) {
    current_source = procedure.signature();

    for (const auto &variable : procedure.args) {
        variables[get_variable_signature(variable.identifier)] =
            VariableDeclaration{.id = get_next_variable_id(), .is_pointer = true};
    }

    push_instruction(Label{}, procedure.signature());

    emit_context(procedure.context);
}

void AstToHir::emit_context(const ast::Context &context) {
    for (const auto &variable : context.declarations) {
        variables[get_variable_signature(variable.identifier)] =
            VariableDeclaration{.id = get_next_variable_id(), .is_pointer = false};
    }

    emit_commands(context.commands);
}

void AstToHir::emit_commands(const std::span<const ast::Command> &commands) {
    for (const auto &command : commands) {
        std::visit(overloaded{
                       [&](const ast::Assignment &assignment) { emit_assignment(assignment); },
                       [&](const ast::Read &read) { push_instruction(Read{get_variable(read.identifier)}); },
                       [&](const ast::Write &write) { emit_write(write); },
                       [&](const ast::While &while_) {},
                       [&](const ast::Call &call) {},
                       [&](const ast::If &if_) {},
                       [&](const ast::Repeat &repeat) {},
                   },
                   command);
    }
}

void AstToHir::emit_write(const ast::Write &write) {
    std::visit(
        overloaded{[&](const ast::Num &num) {
                       const auto constant = get_constant(num);
                       push_instruction(Write{constant});
                   },
                   [&](const ast::Identifier &identifier) { push_instruction(Write{get_variable(identifier)}); }},
        write.value);
}

void AstToHir::emit_assignment(const ast::Assignment &assignment) {
    const auto destination = get_variable(assignment.identifier);
    if (std::holds_alternative<ast::Value>(assignment.expression)) {
        const auto operand = get_operand(std::get<ast::Value>(assignment.expression));
        push_instruction(Assign{destination, operand});
        return;
    }

    const auto binary_expression = std::get<ast::BinaryExpression>(assignment.expression);
    const auto lhs = get_operand(binary_expression.lhs);
    const auto rhs = get_operand(binary_expression.rhs);
}

void AstToHir::push_instruction(HighLevelIRInstruction instruction, std::optional<std::string> label) {
    ir.instructions.push_back(instruction);
}

auto AstToHir::get_variable_signature(const ast::Identifier &identifier) const -> std::string {
    return current_source + "@" + identifier.name.lexeme;
}

auto AstToHir::get_variable_signature(const Token &identifier) const -> std::string {
    return current_source + "@" + identifier.lexeme;
}

auto AstToHir::get_variable_declaration(const Token &pidentifier) const -> VariableDeclaration const * {
    const auto declaration = &variables.at(get_variable_signature(pidentifier));
    return declaration;
}

auto AstToHir::get_variable(const ast::Identifier &identifier) const -> Variable {
    const auto declaration = get_variable_declaration(identifier.name);
    const auto offset = identifier.index ? std::stoull(identifier.index->lexeme) : 0;
    return Variable{.id = declaration->id, .offset = offset};
}

auto AstToHir::get_constant(const ast::Num &num) -> uint64_t {
    const auto constant = std::stoull(num.lexeme);
    ir.constant_frequencies[constant]++;
    return constant;
}

auto AstToHir::get_operand(const ast::Value &value) -> Operand {
    if (std::holds_alternative<ast::Num>(value)) {
        return get_constant(std::get<ast::Num>(value));
    }
    return get_variable(std::get<ast::Identifier>(value));
}

auto AstToHir::get_next_variable_id() -> uint64_t { return next_variable_id++; }
