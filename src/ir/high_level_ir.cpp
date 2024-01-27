#include "high_level_ir.hpp"
template <class> inline constexpr bool always_false_v = false;

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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

    push_instruction(BeginProcedure{}, procedure.signature());

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
                       [&](const ast::Assignment &assignment) {},
                       [&](const ast::Read &read) { push_instruction(Read{get_variable(read.identifier)}); },
                       [&](const ast::Write &write) {},
                       [&](const ast::While &while_) {},
                       [&](const ast::Call &call) {},
                       [&](const ast::If &if_) {},
                       [&](const ast::Repeat &repeat) {},
                   },
                   command);
    }
}

void AstToHir::push_instruction(HighLevelIRInstruction instruction, std::optional<std::string> label) {
    if (label) {
        ir.labels[*label] = ir.instructions.size();
    }

    ir.instructions.push_back(instruction);
}

auto AstToHir::get_variable_signature(const ast::Identifier &identifier) const -> std::string {
    return current_source + "@" + identifier.name.lexeme;
}

auto AstToHir::get_variable_signature(const Token &identifier) const -> std::string {
    return current_source + "@" + identifier.lexeme;
}
