#include "emitter.hpp"
#include "ast.hpp"
#include <format>
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
                       Procedure{entrypoint, stack_pointer, &procedure});

    // we put the return address here
    const auto return_address = stack_pointer;
    stack_pointer++;

    for (const auto &arg : procedure.args) {
        variables[Variable{current_source, arg.identifier.lexeme}] =
            MemoryLocation{stack_pointer, 1, true};
        stack_pointer++;
    }

    assign_memory(procedure.context.declarations);

    // Set the return address
    emit_line_with_comment(Inc{Register::H}, std::format("procedure {}", procedure.signature()));
    emit_line(Inc{Register::H});
    emit_line(Get{Register::H});

    set_memory(return_address);

    // TODO: Handle pointers
    for (const auto &command : procedure.context.commands) {
        emit_command(command);
    }

    set_mar(return_address);
    emit_line(Load{Register::B});
    emit_line_with_comment(
        Jumpr{Register::A},
        std::format("return from procedure {}", procedure.signature()));
}

auto Emitter::get_variable(const std::string &name) -> Location * {
    if (!variables.contains({current_source, name})) {
        return nullptr;
    }
    return &variables[{current_source, name}];
}

bool Emitter::is_pointer(const std::string &name) {
    const auto var = get_variable(name);

    if (!var) {
        return false;
    }

    return var->is_pointer;
}

void Emitter::set_mar(uint64_t value) { set_register(Register::B, value); }

/// Sets the value of register B (B <- id)
void Emitter::set_mar(const parser::Identifier &identifier) {
    const auto location = variables[{current_source, identifier.name.lexeme}];

    auto offset = 0u;

    bool indexed = false;

    if (identifier.index.has_value()) {
        // TODO: Handle indexing by variable
        switch (identifier.index->token_type) {
        case Pidentifier: {
            emit_line_with_comment(Get{Register::F}, " <- backup A (1)");
            const auto &variable = get_variable(identifier.index->lexeme);
            // NOTE: Can potentially break if variable is a pointer (?)
            set_mar(variable->address);
            emit_line(Load{Register::B});
            set_register(Register::C, location.address);
            emit_line(Add{Register::C});
            emit_line(Put{Register::B});

            emit_line_with_comment(Get{Register::F}, " <- backup A (2)");

            indexed = true;
        } break;
        case Num: {
            offset = std::stoull(identifier.index->lexeme);
            if (offset > location.size) {
                // TODO: Add better error;
                std::cerr << "Index outside bounds\n";
                assert(false);
            }
        } break;
        default:
            std::cerr << "Invalid index\n";
            assert(false);
            break;
        }
    }

    if (!indexed)
        set_register(Register::B, location.address + offset);

    if (is_pointer(identifier.name.lexeme)) {
        emit_line_with_comment(Put{Register::G}, " <- pointer");
        emit_line(Load{Register::B});
        emit_line(Put{Register::B});
        emit_line(Get{Register::G});
    }
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
        emit_line(Load{Register::B});
    }
}

/// Store the value in the accumulator in the specified address
/// (memory[address] <- A)
void Emitter::set_memory(uint64_t address) {
    set_mar(address);
    emit_line(Store{Register::B});
}

/// Store the value in the accumulator in the memory location specified by
/// the identifier (memory[B] <- A)
void Emitter::set_memory(const parser::Identifier &identifier) {
    set_mar(identifier);
    emit_line(Store{Register::B});
}

void Emitter::emit_read(const parser::Identifier &identifier) {
    push_comment(Comment{"READ " + identifier.name.lexeme});
    emit_line(Read{});

    set_memory(identifier);
}

void Emitter::emit_write(const parser::Value &value) {
    if (std::holds_alternative<parser::Num>(value)) {
        const auto number = std::get<parser::Num>(value);
        push_comment(Comment{"WRITE " + number.lexeme});
        set_accumulator(value);
    } else {
        const auto identifier = std::get<parser::Identifier>(value);
        push_comment(Comment{"WRITE " + identifier.name.lexeme});
        set_mar(identifier);
        emit_line(Load{Register::B});
    }
    emit_line(Write{});
}

void Emitter::emit_assignment(const parser::Assignment &assignment) {
    const auto lhs_comment = assignment.identifier.get_str() + " = ";

    if (std::holds_alternative<parser::Value>(assignment.expression)) {
        const auto &value = std::get<parser::Value>(assignment.expression);

        set_accumulator(value);
        set_memory(assignment.identifier);
        return;
    }

    auto check_regd_geq_zero = [&](uint64_t offset) {
        const auto jump_line = lines.size();
        emit_line(Get{Register::D});
        emit_line_with_comment(Jzero{lines.size() + offset},
                               "Jump to end if reg D == 0");
        return jump_line;
    };

    const auto &binary =
        std::get<parser::BinaryExpression>(assignment.expression);

    auto gen_comment = [&](const char op) {
        push_comment(Comment{lhs_comment + get_str(binary.lhs) + " " + op +
                                 " " + get_str(binary.rhs),
                             indent_level1});
    };

    switch (binary.op.token_type) {
    case TokenType::Plus: {
        gen_comment('+');
        set_register(Register::C, binary.rhs);
        set_accumulator(binary.lhs);
        emit_line(Add{Register::C});
    } break;
    case TokenType::Minus:
        gen_comment('-');
        set_register(Register::C, binary.rhs);
        set_accumulator(binary.lhs);
        emit_line(Sub{Register::C});
        break;
    case TokenType::Star: {
        gen_comment('*');
        // Register C <- a
        // Register D <- b
        // Register F <- acc
        set_register(Register::C, binary.lhs);
        set_register(Register::D, binary.rhs);

        emit_line(Rst{Register::F});

        const auto cond_line = check_regd_geq_zero(12);

        // Check parity
        emit_line(Shr{Register::A});
        emit_line(Shl{Register::A});
        emit_line(Inc{Register::A});
        emit_line(Sub{Register::D});
        const auto if_false_jump = lines.size() + 4;
        // If not even, jump to endif
        emit_line(Jpos{if_false_jump});

        // otherwise p += a
        emit_line(Get{Register::F});
        emit_line(Add{Register::C});
        emit_line(Put{Register::F});

        emit_line(Shl{Register::C});
        emit_line(Shr{Register::D});
        emit_line(Jump{cond_line});

        emit_line(Get{Register::F});
    } break;
    case TokenType::Slash: {
        gen_comment('/');
        // Register C <- a
        // Register D <- b
        // Register F <- p
        // Register E <- tmp

        set_register(Register::C, binary.lhs);
        set_register(Register::D, binary.rhs);

        emit_line(Rst{Register::F});
        emit_line(Rst{Register::E});

        const auto len = 21;
        // test if b = 0
        emit_line(Get{Register::D});
        emit_line_with_comment(Jzero{lines.size() + len},
                               "Jump to end if reg D == 0");
        // endif

        // tmp := 1
        emit_line(Inc{Register::E});

        // while b <= a condition
        emit_line(Get{Register::D});
        emit_line(Sub{Register::C});
        emit_line_with_comment(Jpos{lines.size() + 4}, "jump if a < b");
        // tmp <<= 1, b <<= 1
        emit_line(Shl{Register::E});
        emit_line(Shl{Register::D});
        emit_line(Jump{lines.size() - 5});
        // endwhile

        // repeat
        const auto repeat_until_entry = lines.size();
        // if b <= a
        emit_line(Get{Register::D});
        emit_line(Sub{Register::C});
        emit_line_with_comment(Jpos{lines.size() + 7}, "jump if a < b");
        // p+=tmp
        emit_line(Get{Register::F});
        emit_line(Add{Register::E});
        emit_line(Put{Register::F});
        // a -= b
        emit_line(Get{Register::C});
        emit_line(Sub{Register::D});
        emit_line(Put{Register::C});
        // endif

        // b >>= 1, tmp >>= 1
        emit_line(Shr{Register::D});
        emit_line(Shr{Register::E});

        emit_line(Get{Register::E});
        emit_line_with_comment(Jpos{repeat_until_entry}, "jump if a < b");

        emit_line(Get{Register::F});
    } break;
    case TokenType::Percent: {
        gen_comment('%');
        // Register C <- a
        // Register D <- b
        // Register F <- p
        // Register E <- tmp

        set_register(Register::C, binary.lhs);
        set_register(Register::D, binary.rhs);

        emit_line_with_comment(Rst{Register::F}, "p := 0");

        const auto len = 21;
        // test if b = 0
        emit_line(Get{Register::D});
        emit_line_with_comment(Jzero{lines.size() + len},
                               "Jump to end if reg D == 0");
        // endif

        // tmp := b
        emit_line_with_comment(Get{Register::D}, "tmp := b");
        emit_line(Put{Register::E});

        // while b <= a condition
        emit_line_with_comment(Get{Register::D}, "check b <= a");
        emit_line(Sub{Register::C});
        emit_line_with_comment(Jpos{lines.size() + 3}, "jump if a < b");
        // b <<= 1
        emit_line(Shl{Register::D});
        emit_line(Jump{lines.size() - 4});
        // endwhile

        // repeat
        const auto repeat_until_entry = lines.size();
        // b >>= 1
        emit_line_with_comment(Shr{Register::D}, "b >>= 1");
        // if b <= a
        emit_line_with_comment(Get{Register::D}, "Check b <= a");
        emit_line(Sub{Register::C});
        emit_line_with_comment(Jpos{lines.size() + 4}, "jump if a < b");
        // a -= b
        emit_line(Get{Register::C});
        emit_line(Sub{Register::D});
        emit_line(Put{Register::C});
        // endif

        // while tmp <= a
        emit_line_with_comment(Get{Register::E}, "Check tmp <= a");
        emit_line(Sub{Register::C});
        emit_line_with_comment(Jzero{repeat_until_entry}, "jump if a < tmp");

        emit_line(Get{Register::C});
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
        emit_line(Sub{Register::C});
    };

    push_comment(Comment{"Condition:", indent_level2});

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

    push_comment(Comment{"If statement"});

    const auto body_start = lines.size();

    push_comment(Comment{"Body:", indent_level2});
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
        push_comment(Comment{"Else Body:", indent_level2});
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
    push_comment(Comment{"Repeat statement"});

    const auto body_start = lines.size();

    for (const auto &command : repeat.commands) {
        emit_command(command);
    }

    // Emit the condition
    const std::string if_false_comment = "Jump to repeat end";

    const auto [jumps_if_false, jumps_if_true] =
        emit_condition(repeat.condition, if_false_comment);

    const auto body_end = lines.size();

    for (auto i : jumps_if_false) {
        set_jump_location(lines[i].instruction, body_start);
    }

    for (auto i : jumps_if_true) {
        set_jump_location(lines[i].instruction, body_end);
    }
}

void Emitter::emit_while(const parser::While &while_statement) {
    push_comment(Comment{"While statement"});

    // Emit the condition
    const std::string if_false_comment = "Jump to while end";

    const auto condition_start = lines.size();

    const auto [jumps_if_false, jumps_if_true] =
        emit_condition(while_statement.condition, if_false_comment);

    const auto body_start = lines.size();

    for (const auto &command : while_statement.commands) {
        emit_command(command);
    }

    emit_line_with_comment(Jump{condition_start}, "Jump to condition");

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

    push_comment(Comment{"Call " + call.name.lexeme});

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

    const auto procedure_memory_entry = procedures[call.name.lexeme].memory_loc;

    set_register(Register::G, procedure_memory_entry + 1);

    for (auto i = 0u; i < num_args; i++) {
        // Check if variable exists
        if (!variables.contains({previous_source, call.args[i].lexeme})) {
            std::cerr << "Variable " << call.args[i].lexeme << " not found"
                      << std::endl;
            exit(1);
        }

        const auto variable_mem_location =
            variables[{previous_source, call.args[i].lexeme}];

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

        if (variables[{previous_source, call.args[i].lexeme}].is_pointer) {
            set_register(Register::B, variable_mem_location.address);
            emit_line(Load{Register::B});
        } else {
            set_accumulator(variable_mem_location.address);
        }

        emit_line(Store{Register::G});
        emit_line(Inc{Register::G});
    }

    emit_line_with_comment(Strk{Register::H}, "Save return address");
    emit_line_with_comment(Jump{procedures[call.name.lexeme].entrypoint},
                           "Jump to procedure " + call.name.lexeme);

    current_source = previous_source;
}

void Emitter::set_register(Register reg, uint64_t value) {
    const auto register_str =
        reg == Register::B ? "MAR(reg B)" : "Reg " + to_string(reg);

    emit_line_with_comment(Rst{reg}, "\t" + register_str + " <- " +
                                         std::to_string(value));

    if (value == 0) {
        return;
    }

    auto value_copy = value;
    unsigned msb_position = 0;

    while (value_copy >>= 1) {
        msb_position++;
    }

    uint64_t mask = 1llu << msb_position;

    while (mask > 1) {
        if (value & mask) {
            emit_line(Inc{reg});
        }
        emit_line(Shl{reg});
        mask >>= 1;
    }

    if (value & mask) {
        emit_line(Inc{reg});
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
        emit_line(Load{Register::B});
        emit_line(Put{reg});
    }
}

// First 8 memory cells are for register backups
void Emitter::backup_register(Register reg) {
    const auto memory_offset = static_cast<uint64_t>(reg);
    emit_line_with_comment(Load{reg}, "Backup register " + to_string(reg));
    set_register(Register::B, memory_offset);
    emit_line(Store{Register::B});
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

    emit_line_with_comment(Halt{}, "Halt");
}

void Emitter::emit_line(const Instruction &instruction) {
    auto comment = std::string{};
    if (!comments.empty()) {
        comment = comments.top().get_str();
        comments.pop();
    }
    lines.push_back(Line{instruction, comment});
}
void Emitter::push_comment(const Comment &comment) { comments.push(comment); }

void Emitter::emit_line_with_comment(const Instruction &instruction,
                                     const std::string &comment) {
    lines.push_back(Line{instruction, comment});
}
