#include "emitter.hpp"

using namespace emitter;

void Emitter::emit_procedure(const parser::Procedure &procedure) {}

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

void Emitter::set_memory(const parser::Identifier &identifier) {
    set_mar(identifier);
    lines.push_back(Line{Store{Register::B}});
}

void Emitter::emit_read(const parser::Identifier &identifier) {
    lines.push_back(Line{Read{}, "Read into " + identifier.name.lexeme});

    set_memory(identifier);
}

void Emitter::emit_write(const parser::Value &value) {
    if (std::holds_alternative<parser::Num>(value)) {
        const auto number = std::get<parser::Num>(value);
        set_register(Register::B, std::stoull(number.lexeme));
        lines.push_back(Line{Write{}, "Write " + number.lexeme});
    } else {
        const auto identifier = std::get<parser::Identifier>(value);
        set_mar(identifier);
        lines.push_back(Line{Load{Register::B}});
        lines.push_back(Line{Write{}, "Write " + identifier.name.lexeme});
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

void Emitter::emit() {
    // for (const auto &procedure : program.procedures) {
    //     emit_procedure(procedure);
    // }

    assign_var_memory();

    for (const auto &command : program.main.commands) {
        std::visit(
            overloaded{
                [&](const parser::Read &read) { emit_read(read.identifier); },
                [&](const parser::Write &write) { emit_write(write.value); },
                [&](auto arg) { assert(false); }},
            command);
    }

    lines.push_back(Line{Halt{}, "Halt"});
}
