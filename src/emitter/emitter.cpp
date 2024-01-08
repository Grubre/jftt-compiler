#include "emitter.hpp"
#include "ast.hpp"
#include <iostream>
#include <limits>
#include <string>

using namespace emitter;

constexpr auto indent_level1 = 10;
constexpr auto indent_level2 = 15;
constexpr auto indent_level3 = 20;

/// Before calling the procedure, register H must be set to the return address
void Emitter::emit_procedure(const parser::Procedure &procedure) {
    // TODO: Add error checking e.g
    // - check if procedure name is unique
    // - check if declarations and arguments are unique

    current_source = procedure.name.lexeme;

    const auto entrypoint = lines.size();

    procedures.emplace(procedure.name.lexeme,
                       Procedure{entrypoint, &procedure});

    for (const auto &arg : procedure.args) {
        variables[Variable{current_source, arg.identifier.lexeme}] =
            MemoryLocation{stack_pointer, std::numeric_limits<uint64_t>::max()};
        stack_pointer++;
    }

    assign_memory(procedure.context.declarations);

    for (const auto &command : procedure.context.commands) {
        emit_command(command);
    }

    lines.push_back(Line{Jumpr{Register::H}, "Return to caller"});
}

void Emitter::emit_comment(const Comment &comment) {
    // lines.push_back(Line{comment});
}

void Emitter::set_mar(uint64_t value) { set_register(Register::B, value); }

/// Sets the value of register B (B <- id)
void Emitter::set_mar(const parser::Identifier &identifier) {
    const auto location = variables[{current_source, identifier.name.lexeme}];
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

void Emitter::set_accumulator(uint64_t value) {
    set_register(Register::A, value);
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

/// Store the value in the accumulator in the specified address
/// (memory[address] <- A)
void Emitter::set_memory(uint64_t address) {
    set_mar(address);
    lines.push_back(Line{Store{Register::B}});
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

void Emitter::emit_assignment(const parser::Assignment &assignment) {
    if (std::holds_alternative<parser::Value>(assignment.expression)) {
        const auto &value = std::get<parser::Value>(assignment.expression);

        set_accumulator(value);
        set_memory(assignment.identifier);
        return;
    }

    auto check_regd_geq_zero = [&](uint64_t offset) {
        const auto jump_line = lines.size();
        lines.push_back(Line{Get{Register::D}});
        lines.push_back(
            Line{Jzero{lines.size() + offset}, "Jump to end if reg D == 0"});
        return jump_line;
    };

    const auto &binary =
        std::get<parser::BinaryExpression>(assignment.expression);

    switch (binary.op.token_type) {
    case TokenType::Plus: {
        set_register(Register::C, binary.rhs);
        set_accumulator(binary.lhs);
        lines.push_back(Line{Add{Register::C}});
    } break;
    case TokenType::Minus:
        set_register(Register::C, binary.rhs);
        set_accumulator(binary.lhs);
        lines.push_back(Line{Sub{Register::C}});
        break;
    case TokenType::Star: {
        // Register C <- a
        // Register D <- b
        // Register F <- acc
        set_register(Register::C, binary.lhs);
        set_register(Register::D, binary.rhs);

        lines.push_back(Line{Rst{Register::F}}); // p

        const auto cond_line = check_regd_geq_zero(12);

        // Check parity
        lines.push_back(Line{Shr{Register::A}});
        lines.push_back(Line{Shl{Register::A}});
        lines.push_back(Line{Inc{Register::A}});
        lines.push_back(Line{Sub{Register::D}});
        const auto if_false_jump = lines.size() + 4;
        // If not even, jump to endif
        lines.push_back(Line{Jpos{if_false_jump}});

        // otherwise p += a
        lines.push_back(Line{Get{Register::F}});
        lines.push_back(Line{Add{Register::C}});
        lines.push_back(Line{Put{Register::F}});

        lines.push_back(Line{Shl{Register::C}});
        lines.push_back(Line{Shr{Register::D}});
        lines.push_back(Line{Jump{cond_line}});

        lines.push_back(Line{Get{Register::F}});
    } break;
    case TokenType::Slash: {
        // Register C <- a
        // Register D <- b
        // Register F <- acc

        set_register(Register::C, binary.lhs);
        set_register(Register::D, binary.rhs);

        lines.push_back(Line{Rst{Register::F}}); // p
        lines.push_back(Line{Rst{Register::E}}); // tmp

        const auto len = 21;
        // test if b = 0
        lines.push_back(Line{Get{Register::D}});
        lines.push_back(
            Line{Jzero{lines.size() + len}, "Jump to end if reg D == 0"});
        // endif

        // tmp := 1
        lines.push_back(Line{Inc{Register::E}});

        // while b <= a condition
        lines.push_back(Line{Get{Register::D}});
        lines.push_back(Line{Sub{Register::C}});
        lines.push_back(Line{Jpos{lines.size() + 4}, "jump if a < b"});
        // tmp <<= 1, b <<= 1
        lines.push_back(Line{Shl{Register::E}});
        lines.push_back(Line{Shl{Register::D}});
        lines.push_back(Line{Jump{lines.size() - 5}});
        // endwhile

        // repeat
        const auto repeat_until_entry = lines.size();
        // if b <= a
        lines.push_back(Line{Get{Register::D}});
        lines.push_back(Line{Sub{Register::C}});
        lines.push_back(Line{Jpos{lines.size() + 7}, "jump if a < b"});
        // p+=tmp
        lines.push_back(Line{Get{Register::F}});
        lines.push_back(Line{Add{Register::E}});
        lines.push_back(Line{Put{Register::F}});
        // a -= b
        lines.push_back(Line{Get{Register::C}});
        lines.push_back(Line{Sub{Register::D}});
        lines.push_back(Line{Put{Register::C}});
        // endif

        // b >>= 1, tmp >>= 1
        lines.push_back(Line{Shr{Register::D}});
        lines.push_back(Line{Shr{Register::E}});

        lines.push_back(Line{Get{Register::E}});
        lines.push_back(Line{Jpos{repeat_until_entry}, "jump if a < b"});

        lines.push_back(Line{Get{Register::F}});
    } break;
    case TokenType::Percent: {
        // Register C <- a
        // Register D <- b
        // Register F <- p
        // Register E <- tmp

        set_register(Register::C, binary.lhs);
        set_register(Register::D, binary.rhs);

        lines.push_back(Line{Rst{Register::F}, "p := 0"}); // p

        const auto len = 21;
        // test if b = 0
        lines.push_back(Line{Get{Register::D}});
        lines.push_back(
            Line{Jzero{lines.size() + len}, "Jump to end if reg D == 0"});
        // endif

        // tmp := b
        lines.push_back(Line{Get{Register::D}, "tmp := b"});
        lines.push_back(Line{Put{Register::E}});

        // while b <= a condition
        lines.push_back(Line{Get{Register::D}, "check b <= a"});
        lines.push_back(Line{Sub{Register::C}});
        lines.push_back(Line{Jpos{lines.size() + 3}, "jump if a < b"});
        // b <<= 1
        lines.push_back(Line{Shl{Register::D}});
        lines.push_back(Line{Jump{lines.size() - 4}});
        // endwhile

        // repeat
        const auto repeat_until_entry = lines.size();
        // b >>= 1
        lines.push_back(Line{Shr{Register::D}, "b >>= 1"});
        // if b <= a
        lines.push_back(Line{Get{Register::D}, "Check b <= a"});
        lines.push_back(Line{Sub{Register::C}});
        lines.push_back(Line{Jpos{lines.size() + 4}, "jump if a < b"});
        // a -= b
        lines.push_back(Line{Get{Register::C}});
        lines.push_back(Line{Sub{Register::D}});
        lines.push_back(Line{Put{Register::C}});
        // endif

        // while tmp <= a
        lines.push_back(Line{Get{Register::E}, "Check tmp <= a"});
        lines.push_back(Line{Sub{Register::C}});
        lines.push_back(Line{Jzero{repeat_until_entry}, "jump if a < tmp"});

        lines.push_back(Line{Get{Register::C}});
    }

    break;
    default:
        std::cerr << "Operator " << binary.op.lexeme << " not implemented"
                  << std::endl;
        assert(false);
    }

    set_memory(assignment.identifier);
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

void Emitter::emit_repeat(const parser::Repeat &repeat) {
    emit_comment(Comment{"Repeat statement"});

    const auto body_start = lines.size();

    for (const auto &command : repeat.commands) {
        emit_command(command);
    }

    // Emit the condition
    const std::string if_false_comment = "Jump to repeat end";

    const auto [jumps_if_false, jumps_if_true] =
        emit_condition(repeat.condition, if_false_comment);

    const auto body_end = lines.size();

    for (auto i : jumps_if_true) {
        set_jump_location(lines[i].instruction, body_start);
    }

    for (auto i : jumps_if_false) {
        set_jump_location(lines[i].instruction, body_end);
    }
}

void Emitter::emit_while(const parser::While &while_statement) {
    emit_comment(Comment{"While statement"});

    // Emit the condition
    const std::string if_false_comment = "Jump to while end";

    const auto condition_start = lines.size();

    const auto [jumps_if_false, jumps_if_true] =
        emit_condition(while_statement.condition, if_false_comment);

    const auto body_start = lines.size();

    for (const auto &command : while_statement.commands) {
        emit_command(command);
    }

    lines.push_back(Line{Jump{condition_start}, "Jump to condition"});

    const auto body_end = lines.size();

    for (auto i : jumps_if_true) {
        set_jump_location(lines[i].instruction, body_start);
    }

    for (auto i : jumps_if_false) {
        set_jump_location(lines[i].instruction, body_end);
    }
}

void Emitter::emit_call(const parser::Call &call) {
    const auto num_args = call.args.size();

    emit_comment(Comment{"Call " + call.name.lexeme});

    if (!procedures.contains(call.name.lexeme)) {
        // TODO: Add better error handling using struct Error
        std::cerr << "Procedure " << call.name.lexeme << " not found"
                  << std::endl;
        assert(false);
    }

    if (num_args != procedures[call.name.lexeme].procedure->args.size()) {
        std::cerr << "Procedure " << call.name.lexeme << " expected "
                  << procedures[call.name.lexeme].procedure->args.size()
                  << " arguments but got " << num_args << std::endl;
        assert(false);
    }

    const auto previous_source = current_source;

    current_source = call.name.lexeme;

    const auto &procedure = procedures[call.name.lexeme].procedure;

    for (auto i = 0u; i < num_args; i++) {
        const auto variable_mem_location =
            variables[{current_source, call.args[i].lexeme}];

        const auto inout_mem_location =
            inouts[{current_source, procedure->args[i].identifier.lexeme}];

        if (variable_mem_location.size == 1 && procedure->args[i].is_array) {
            // TODO: Better errors
            std::cerr << "Procedure " << call.name.lexeme
                      << " expected an array but got a variable" << std::endl;
            assert(false);
        }

        if (variable_mem_location.size > 1 && !procedure->args[i].is_array) {
            // TODO: Better errors
            std::cerr << "Procedure " << call.name.lexeme
                      << " expected a variable but got an array" << std::endl;
            assert(false);
        }

        set_accumulator(variable_mem_location.address);
        set_memory(inout_mem_location);
    }

    lines.push_back(Line{Jump{procedures[call.name.lexeme].entrypoint},
                         "Jump to procedure " + call.name.lexeme});

    current_source = previous_source;
}

void Emitter::set_register(Register reg, uint64_t value) {
    const auto register_str =
        reg == Register::B ? "MAR(reg B)" : "Reg " + to_string(reg);

    lines.push_back(
        Line{Rst{reg}, "\t" + register_str + " <- " + std::to_string(value)});

    if (value == 0) {
        return;
    }
    lines.push_back(Line{Inc{reg}});
    uint64_t acc = 1;
    while (2 * acc <= value) {
        lines.push_back(Line{Shl{reg}});
        acc *= 2;
    }

    while (acc < value) {
        lines.push_back(Line{Inc{reg}});
        acc++;
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

void Emitter::assign_memory(
    const std::vector<parser::Declaration> &declarations) {
    for (const auto &declaration : declarations) {
        const auto size = declaration.array_size.has_value()
                              ? std::stoull(declaration.array_size->lexeme)
                              : 1;
        variables[{current_source, declaration.identifier.lexeme}] =
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
            [&](const parser::Repeat &repeat) { emit_repeat(repeat); },
            [&](const parser::Assignment &assignment) {
                emit_assignment(assignment);
            },
            [&](const parser::While &while_statement) {
                emit_while(while_statement);
            },
            [&](const parser::Call &call) { emit_call(call); },
            [&](auto arg) { assert(false); }},
        command);
}

void Emitter::emit() {
    for (const auto &procedure : program.procedures) {
        emit_procedure(procedure);
    }

    current_source = "PROGRAM";

    assign_memory(program.main.declarations);

    // Jump to main
    std::get<Jump>(lines[0].instruction).line = lines.size();

    for (const auto &command : program.main.commands) {
        emit_command(command);
    }

    lines.push_back(Line{Halt{}, "Halt"});
}
