#include "emitter.hpp"
#include "ast.hpp"
#include <iostream>
#include <string>

using namespace emitter;

constexpr auto indent_level1 = 10;
constexpr auto indent_level2 = 15;
constexpr auto indent_level3 = 20;

void Emitter::emit_procedure(const parser::Procedure &procedure) {}

void Emitter::emit_comment(const Comment &comment) {
    // lines.push_back(Line{comment});
}

void Emitter::set_mar(const parser::Identifier &identifier) {
    const auto location = variables[identifier.name.lexeme];
    auto offset = 0u;

    if (identifier.index.has_value()) {
        // TODO: Handle indexing by variable
        offset = std::stoull(identifier.index->lexeme);
        // TODO: check if offset is within bounds
    }

    set_register(Register::B, location.address + offset);
}

void Emitter::set_jump_location(Instruction &instruction, uint64_t location) {
    std::visit(overloaded{[&](Jump &jump) { jump.line = location; },
                          [&](Jpos &jpos) { jpos.line = location; },
                          [&](Jzero &jzero) { jzero.line = location; },
                          [&](auto arg) { assert(false); }},
               instruction);
}

void Emitter::set_accumulator(const parser::Value &value) {
    if (std::holds_alternative<parser::Num>(value)) {
        const auto number = std::get<parser::Num>(value);
        set_register(Register::A, std::stoull(number.lexeme));
    } else {
        const auto identifier = std::get<parser::Identifier>(value);
        set_mar(identifier);
        lines.push_back(Line{Load{Register::B}});
    }
}

/// Store the value in the accumulator in the memory location specified by the
/// identifier (memory[B] <- A)
void Emitter::set_memory(const parser::Identifier &identifier) {
    set_mar(identifier);
    lines.push_back(Line{Store{Register::B}});
}

void Emitter::emit_read(const parser::Identifier &identifier) {
    emit_comment(Comment{"Read into " + identifier.name.lexeme});
    lines.push_back(Line{Read{}});

    set_memory(identifier);
}

void Emitter::emit_write(const parser::Value &value) {
    if (std::holds_alternative<parser::Num>(value)) {
        const auto number = std::get<parser::Num>(value);
        emit_comment(Comment{"Write " + number.lexeme});
        set_accumulator(value);
    } else {
        const auto identifier = std::get<parser::Identifier>(value);
        emit_comment(Comment{"Write " + identifier.name.lexeme});
        set_mar(identifier);
        lines.push_back(Line{Load{Register::B}});
    }
    lines.push_back(Line{Write{}});
}

void Emitter::emit_assignment(const parser::Identifier &identifier,
                              const parser::Expression &expression) {
    if (std::holds_alternative<parser::Value>(expression)) {
        const auto &value = std::get<parser::Value>(expression);

        set_accumulator(value);
    } else {
        const auto &binary = std::get<parser::BinaryExpression>(expression);

        set_accumulator(binary.lhs);
        set_register(Register::C, binary.rhs);

        switch (binary.op.token_type) {
        case TokenType::Plus:
            lines.push_back(Line{Add{Register::C}});
            break;
        case TokenType::Minus:
            lines.push_back(Line{Sub{Register::C}});
            break;
        default:
            std::cerr << "Operator " << binary.op.lexeme << " not implemented"
                      << std::endl;
            assert(false);
        }
    }

    set_memory(identifier);
}

auto Emitter::emit_condition(const parser::Condition &condition,
                             const std::string &comment_when_false) -> Jumps {
    auto jumps_if_false = std::vector<uint64_t>{};
    auto jumps_if_true = std::vector<uint64_t>{};

    auto jump_if_false = [&](Line line) {
        lines.push_back(line);
        jumps_if_false.push_back(lines.size() - 1);
    };

    auto jump_if_true = [&](Line line) {
        lines.push_back(line);
        jumps_if_true.push_back(lines.size() - 1);
    };

    auto subtract = [&](Register lhs, Register rhs) {
        if (lhs == Register::A) {
            set_register(rhs, condition.rhs);
            set_register(lhs, condition.lhs);
        } else {
            set_register(lhs, condition.lhs);
            set_register(rhs, condition.rhs);
        }
        lines.push_back(Line{Sub{Register::C}});
    };

    emit_comment(Comment{"Condition:", indent_level2});

    switch (condition.op.token_type) {
    case TokenType::LessEquals:
        subtract(Register::A, Register::C);
        jump_if_false(Line{Jpos{0}, comment_when_false + " if >="});
        break;

    case TokenType::GreaterEquals:
        subtract(Register::C, Register::A);
        jump_if_false(Line{Jpos{0}, comment_when_false + " if <="});
        break;

    case TokenType::Equals:
        // We have to check a >= b and a <= b

        // a >= b
        subtract(Register::A, Register::C);
        jump_if_false(Line{Jpos{1}, comment_when_false + " if > (1)"});

        // a <= b
        subtract(Register::C, Register::A);
        jump_if_false(Line{Jpos{1}, comment_when_false + " if < (2)"});

        break;
    case TokenType::BangEquals:
        // We also check a >= b and a <= b but we only jump to the body
        // at least one is false

        // a >= b
        subtract(Register::A, Register::C);
        jump_if_true(Line{Jpos{1}, comment_when_false + " if > (1)"});

        // a <= b
        subtract(Register::C, Register::A);
        jump_if_true(Line{Jpos{1}, comment_when_false + " if < (2)"});

        jump_if_false(Line{Jump{0}, comment_when_false + " if =="});

        break;
    case TokenType::Less:
        // We check that a <= b but not that a >= b

        // a >= b
        subtract(Register::A, Register::C);
        jump_if_false(Line{Jpos{0}, comment_when_false + " if >="});

        // a <= b
        subtract(Register::C, Register::A);
        jump_if_true(Line{Jpos{0}, "Jump to body if <"});

        jump_if_false(Line{Jump{0}, comment_when_false + " if =="});
        break;
    case TokenType::Greater:
        // We check that a >= b but not that a <= b

        // a <= b
        subtract(Register::C, Register::A);
        jump_if_false(Line{Jpos{0}, comment_when_false + " if <="});

        // a >= b
        subtract(Register::A, Register::C);
        jump_if_true(Line{Jpos{0}, "Jump to body if >"});

        jump_if_false(Line{Jump{0}, comment_when_false + " if =="});
        break;
    default:
        std::cerr << "Operator " << condition.op.lexeme << " not implemented"
                  << std::endl;
        assert(false);
    }

    return {jumps_if_false, jumps_if_true};
}

void Emitter::emit_if(const parser::If &if_statement) {
    auto jumps_to_else_end = std::vector<uint64_t>{};

    auto jump_to_else_end = [&](Line line) {
        lines.push_back(line);
        jumps_to_else_end.push_back(lines.size() - 1);
    };

    // Emit the condition
    const std::string comment =
        std::string{"Jump to "} +
        (if_statement.else_commands.has_value() ? "else" : "endif");

    const auto [jumps_if_false, jumps_if_true] =
        emit_condition(if_statement.condition, comment);

    emit_comment(Comment{"If statement"});

    const auto body_start = lines.size();

    emit_comment(Comment{"Body:", indent_level2});
    for (const auto &command : if_statement.commands) {
        emit_command(command);
    }

    if (if_statement.else_commands.has_value()) {
        jump_to_else_end(Line{Jump{0}, comment + " endif"});
    }

    const auto body_end = lines.size();

    for (auto i : jumps_if_true) {
        set_jump_location(lines[i].instruction, body_start);
    }

    for (auto i : jumps_if_false) {
        set_jump_location(lines[i].instruction, body_end);
    }

    if (if_statement.else_commands.has_value()) {
        emit_comment(Comment{"Else Body:", indent_level2});
        for (const auto &command : if_statement.else_commands.value()) {
            emit_command(command);
        }

        const auto else_end = lines.size();

        for (auto i : jumps_to_else_end) {
            set_jump_location(lines[i].instruction, else_end);
        }
    }
}

void Emitter::set_register(Register reg, uint64_t value) {
    const auto register_str =
        reg == Register::B ? "MAR(reg B)" : "Reg " + to_string(reg);
    lines.push_back(
        Line{Rst{reg}, "\t" + register_str + " <- " + std::to_string(value)});
    for (auto i = 0; i < value; i++) {
        lines.push_back(Line{Inc{reg}});
    }
}

void Emitter::set_register(Register reg, const parser::Value &value) {
    if (reg == Register::A) {
        set_accumulator(value);
        return;
    }
    if (std::holds_alternative<parser::Num>(value)) {
        const auto number = std::get<parser::Num>(value);
        set_register(reg, std::stoull(number.lexeme));
    } else {
        const auto identifier = std::get<parser::Identifier>(value);
        set_mar(identifier);
        lines.push_back(Line{Load{Register::B}});
        lines.push_back(Line{Put{reg}});
    }
}

// First 8 memory cells are for register backups
void Emitter::backup_register(Register reg) {
    const auto memory_offset = static_cast<uint64_t>(reg);
    lines.push_back(Line{Load{reg}, "Backup register " + to_string(reg)});
    set_register(Register::B, memory_offset);
    lines.push_back(Line{Store{Register::B}});
}

void Emitter::assign_var_memory() {
    for (const auto &declaration : program.main.declarations) {
        const auto size = declaration.array_size.has_value()
                              ? std::stoull(declaration.array_size->lexeme)
                              : 1;
        variables[declaration.identifier.lexeme] =
            MemoryLocation{stack_pointer, size};
        stack_pointer += size;
    }
}

void Emitter::emit_command(const parser::Command &command) {
    std::visit(
        overloaded{
            [&](const parser::Read &read) { emit_read(read.identifier); },
            [&](const parser::Write &write) { emit_write(write.value); },
            [&](const parser::If &if_statement) { emit_if(if_statement); },
            [&](auto arg) { assert(false); }},
        command);
}

void Emitter::emit() {
    // for (const auto &procedure : program.procedures) {
    //     emit_procedure(procedure);
    // }

    assign_var_memory();

    for (const auto &command : program.main.commands) {
        emit_command(command);
    }

    lines.push_back(Line{Halt{}, "Halt"});
}
