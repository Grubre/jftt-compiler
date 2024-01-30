#include "low_level_ir_builder.hpp"
#include "common.hpp"
#include "instruction.hpp"
#include <algorithm>
#include <iostream>
#include <stack>

namespace lir {

void LirEmitter::populate_interference_graph(Cfg *cfg) {
    interference_graph.neighbours.resize(next_vregister_id);

    for (const auto &block : cfg->basic_blocks) {
        auto alive_registers = block.live_out;
        for (int i = block.instructions.size() - 1; i >= 0; i--) {
            const auto &instruction = block.instructions[i];
            for (const auto overwrite : overwritten_variables(instruction)) {
                alive_registers.erase(overwrite);
            }
            for (const auto read : read_variables(instruction)) {
                alive_registers.insert(read);
            }
            for (const auto &reg : alive_registers) {
                for (const auto &alive_reg : alive_registers) {
                    if (reg != alive_reg) {
                        interference_graph.neighbours[reg].insert(alive_reg);
                    }
                }
            }
        }
    }

    // print the graph
    for (int i = 0; i < interference_graph.neighbours.size(); i++) {
        std::cout << "Register " << i << " neighbours: ";
        for (const auto &neighbour : interference_graph.neighbours[i]) {
            std::cout << neighbour << ", ";
        }
        std::cout << "\n";
    }
}

void LirEmitter::spill(Cfg *cfg, VirtualRegister vreg) {
    const auto spill_location = get_new_memory_location();

    auto insert_instruction = [&](std::vector<VirtualInstruction> &vec, uint64_t pos, VirtualInstruction instruction) {
        vec.insert(vec.begin() + pos, instruction);
    };

    auto gen_constant = [&](std::vector<VirtualInstruction> &vec, uint64_t pos, uint64_t value) -> uint64_t {
        auto value_copy = value;
        unsigned msb_position = 0;

        auto offset = 0u;

        if (value == 0) {
            return 0;
        }

        while (value_copy >>= 1) {
            msb_position++;
        }

        uint64_t mask = 1llu << msb_position;

        while (mask > 1) {
            if (value & mask) {
                insert_instruction(vec, pos + offset, Inc{vreg});
                offset++;
            }
            insert_instruction(vec, pos + offset, Shl{vreg});
            offset++;
            mask >>= 1;
        }

        if (value & mask) {
            insert_instruction(vec, pos + offset, Inc{vreg});
            offset++;
        }

        return offset;
    };

    for (auto &block : cfg->basic_blocks) {
        for (auto i = 0u; i < block.instructions.size(); ++i) {
            auto &instruction = block.instructions[i];

            const auto read = read_variables(instruction);
            const auto overwritten = overwritten_variables(instruction);

            const auto is_read = std::find(read.begin(), read.end(), vreg) != read.end();
            const auto is_overwritten = std::find(overwritten.begin(), overwritten.end(), vreg) != overwritten.end();

            if (is_read && is_overwritten) {
                const auto tmp = new_vregister();
                const auto spilled_vreg = new_vregister();
                change_vreg(instruction, spilled_vreg);
                insert_instruction(block.instructions, i, Put{tmp});
                const auto offset = gen_constant(block.instructions, i + 1, spill_location);
                insert_instruction(block.instructions, i + 2 + offset, Load{regA});
                insert_instruction(block.instructions, i + 4 + offset, Store{regA});
                insert_instruction(block.instructions, i + 5 + offset, Get{tmp});
                i = i + 5 + offset;
            }

            if (is_read) {
                const auto tmp = new_vregister();
                const auto spilled_vreg = new_vregister();
                change_vreg(instruction, spilled_vreg);
                insert_instruction(block.instructions, i, Put{tmp});
                const auto offset = gen_constant(block.instructions, i + 1, spill_location);
                insert_instruction(block.instructions, i + 2 + offset, Load{regA});
                insert_instruction(block.instructions, i + 4 + offset, Get{tmp});
                i = i + 4 + offset;
                continue;
            }

            if (is_overwritten) {
                const auto tmp = new_vregister();
                const auto spilled_vreg = new_vregister();
                change_vreg(instruction, spilled_vreg);
                insert_instruction(block.instructions, i, Put{tmp});
                const auto offset = gen_constant(block.instructions, i + 1, spill_location);
                insert_instruction(block.instructions, i + 2 + offset, Store{regA});
                insert_instruction(block.instructions, i + 4 + offset, Get{tmp});
                i = i + 4 + offset;
                continue;
            }
        }

        block.live_in.erase(vreg);
        block.live_out.erase(vreg);
    }

    populate_interference_graph(cfg);
}

auto LirEmitter::try_color_graph(Cfg *cfg) -> bool {
    auto active_nodes_count = next_vregister_id;
    auto stack = std::stack<uint64_t>{};
    auto active_nodes = std::vector<bool>(next_vregister_id, true);

    const auto deg = 8u;

    while (active_nodes_count > 0) {
        auto current_node = 1u;

        for (auto i = 1u; i < next_vregister_id; ++i) {
            if (active_nodes[i] && interference_graph.neighbours[i].size() < deg) {
                current_node = i;
                break;
            }
        }

        active_nodes[current_node] = false;
        stack.push(current_node);
        active_nodes_count--;
    }

    while (!stack.empty()) {
        auto current_node = stack.top();
        stack.pop();

        auto available_registers = std::vector<bool>(8, true);
        available_registers[0] = false;
        for (const auto neighbour : interference_graph.neighbours[current_node]) {
            if (active_nodes[neighbour])
                available_registers[(int)assigned_registers[neighbour]] = false;
        }

        auto i = 1u;
        for (; i < deg; i++) {
            if (available_registers[i]) {
                assigned_registers[current_node] = (instruction::Register)i;
                break;
            }
        }

        if (i != deg) {
            active_nodes[current_node] = true;
            assigned_registers[current_node] = (instruction::Register)i;
            continue;
        }

        std::cout << "Spilling register " << current_node << "\n";
        spill(cfg, current_node);
        return false;
    }

    for (auto i = 0u; i < assigned_registers.size(); ++i) {
        std::cout << "Register " << i << " assigned to " << (int)assigned_registers[i] << "\n";
    }

    return true;
}

void LirEmitter::allocate_registers(Cfg *cfg) {
    this->cfg = cfg;

    populate_interference_graph(cfg);

    this->assigned_registers = std::vector<instruction::Register>(next_vregister_id);
    assigned_registers[0] = instruction::Register::A;

    while (!try_color_graph(cfg)) {
        populate_interference_graph(cfg);
        this->assigned_registers = std::vector<instruction::Register>(next_vregister_id);
        assigned_registers[0] = instruction::Register::A;
    }
}

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
        resolved_variables[current_source + "@" + variable.identifier.lexeme] =
            ResolvedVariable{vreg, true, variable.is_array};
        procedures[current_source].args.push_back(vreg);
    }

    push_instruction(Label{current_source});
    emit_context(procedure.context);
    push_instruction(Jumpr{return_address_vreg});
}

void LirEmitter::emit_context(const ast::Context &context) {
    for (const auto &variable : context.declarations) {
        const auto signature = current_source + "@" + variable.identifier.lexeme;
        resolved_variables[signature] = ResolvedVariable{new_vregister(), false, variable.array_size.has_value()};
        allocate_memory(signature, variable.array_size.has_value() ? std::stoull(variable.array_size->lexeme) : 1);
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
        const auto rhs = put_constant_to_vreg_or_get(condition.rhs);
        set_vreg(condition.lhs, regA);
        push_instruction(Sub{rhs});
    };
    const auto rhs_minus_lhs = [&]() {
        const auto rhs = put_constant_to_vreg_or_get(condition.lhs);
        set_vreg(condition.rhs, regA);
        push_instruction(Sub{rhs});
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
    const auto false_label = get_label_str("ELSE");
    const auto endif_label = get_label_str("ENDIF");

    if (if_statement.else_commands) {
        emit_condition(if_statement.condition, true_label, false_label);
    } else {
        emit_condition(if_statement.condition, true_label, endif_label);
    }

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
    const auto &identifier = assignment.identifier;

    if (std::holds_alternative<ast::Value>(assignment.expression)) {
        const auto value = std::get<ast::Value>(assignment.expression);
        set_vreg(value, get_variable(identifier).vregister_id);
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

auto LirEmitter::get_from_vreg_or_load_from_mem(const ast::Identifier &identifier) -> VirtualRegister {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Get{variable.vregister_id});
    } else {
        push_instruction(Load{variable.vregister_id});
    }
    return variable.vregister_id;
}

auto LirEmitter::get_from_vreg_or_load_from_mem(const Token &identifier) -> VirtualRegister {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Get{variable.vregister_id});
    } else {
        push_instruction(Load{variable.vregister_id});
    }
    return variable.vregister_id;
}

auto LirEmitter::set_to_vreg_or_store_to_mem(const ast::Identifier &identifier) -> VirtualRegister {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Get{variable.vregister_id});
    } else {
        push_instruction(Load{variable.vregister_id});
    }
    return variable.vregister_id;
}

auto LirEmitter::set_to_vreg_or_store_to_mem(const Token &identifier) -> VirtualRegister {
    const auto variable = get_variable(identifier);

    if (!variable.is_pointer) {
        push_instruction(Get{variable.vregister_id});
    } else {
        push_instruction(Load{variable.vregister_id});
    }
    return variable.vregister_id;
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
        push_instruction(Put{vreg});
    }
}

void LirEmitter::emit_constant(VirtualRegister vregister, const ast::Num &num) {
    const auto num_value = std::stoull(num.lexeme);

    if (num_value == 0) {
        return;
    }

    auto value_copy = num_value;
    unsigned msb_position = 0;

    while (value_copy >>= 1) {
        msb_position++;
    }

    uint64_t mask = 1llu << msb_position;

    push_instruction(Rst{vregister});

    while (mask > 1) {
        if (num_value & mask) {
            push_instruction(Inc{vregister});
        }
        push_instruction(Shl{vregister});
        mask >>= 1;
    }

    if (num_value & mask) {
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
    return get_from_vreg_or_load_from_mem(std::get<ast::Identifier>(value));
}

auto LirEmitter::get_label_str(const std::string &label) -> std::string {
    return std::format("{}#{}", label, next_label_id++);
}

auto LirEmitter::get_procedure_codes() -> ProcedureCodes { return instructions; }
auto LirEmitter::get_flattened_instructions() -> Instructions {
    auto result = Instructions{};
    for (const auto &[_, instructions] : instructions) {
        result.insert(result.end(), instructions.begin(), instructions.end());
    }
    return result;
}

auto LirEmitter::emit_assembler() -> std::vector<instruction::Line> {
    auto codelines = 0u;
    auto labels = std::unordered_map<std::string, uint64_t>{};
    for (const auto &[_, instructions] : instructions) {
        for (const auto &instruction : instructions) {
            if (!std::holds_alternative<Label>(instruction)) {
                codelines++;
                continue;
            }
            const auto label = std::get<Label>(instruction);
            labels[label.name] = codelines;
        }
    }

    auto result = std::vector<instruction::Line>{};
    for (const auto &[_, instructions] : instructions) {
        for (const auto &instruction : instructions) {
            std::visit(overloaded{
                           [&](const Read &) { result.push_back(instruction::Line{instruction::Read{}}); },
                           [&](const Write &) { result.push_back(instruction::Line{instruction::Write{}}); },
                           [&](const Load &load) {
                               const auto mapped_reg = assigned_registers[load.address];
                               result.push_back(instruction::Line{instruction::Load{mapped_reg}});
                           },
                           [&](const Store &store) {
                               const auto mapped_reg = assigned_registers[store.address];
                               result.push_back(instruction::Line{instruction::Store{mapped_reg}});
                           },
                           [&](const Add &add) {
                               const auto mapped_reg = assigned_registers[add.address];
                               result.push_back(instruction::Line{instruction::Add{mapped_reg}});
                           },
                           [&](const Sub &sub) {
                               const auto mapped_reg = assigned_registers[sub.address];
                               result.push_back(instruction::Line{instruction::Sub{mapped_reg}});
                           },
                           [&](const Get &get) {
                               const auto mapped_reg = assigned_registers[get.address];
                               result.push_back(instruction::Line{instruction::Get{mapped_reg}});
                           },
                           [&](const Put &put) {
                               const auto mapped_reg = assigned_registers[put.address];
                               result.push_back(instruction::Line{instruction::Put{mapped_reg}});
                           },
                           [&](const Rst &rst) {
                               const auto mapped_reg = assigned_registers[rst.address];
                               result.push_back(instruction::Line{instruction::Rst{mapped_reg}});
                           },
                           [&](const Inc &inc) {
                               const auto mapped_reg = assigned_registers[inc.address];
                               result.push_back(instruction::Line{instruction::Inc{mapped_reg}});
                           },
                           [&](const Dec &dec) {
                               const auto mapped_reg = assigned_registers[dec.address];
                               result.push_back(instruction::Line{instruction::Dec{mapped_reg}});
                           },
                           [&](const Shl &shl) {
                               const auto mapped_reg = assigned_registers[shl.address];
                               result.push_back(instruction::Line{instruction::Shl{mapped_reg}});
                           },
                           [&](const Shr &shr) {
                               const auto mapped_reg = assigned_registers[shr.address];
                               result.push_back(instruction::Line{instruction::Shr{mapped_reg}});
                           },
                           [&](const Jump &jump) {
                               const auto location = labels[jump.label];
                               result.push_back(instruction::Line{instruction::Jump{location}});
                           },
                           [&](const Jpos &jpos) {
                               const auto location = labels[jpos.label];
                               result.push_back(instruction::Line{instruction::Jpos{location}});
                           },
                           [&](const Jzero &jzero) {
                               const auto location = labels[jzero.label];
                               result.push_back(instruction::Line{instruction::Jzero{location}});
                           },
                           [&](const Strk &strk) {
                               const auto mapped_reg = assigned_registers[strk.reg];
                               result.push_back(instruction::Line{instruction::Strk{mapped_reg}});
                           },
                           [&](const Jumpr &jumpr) {
                               const auto mapped_reg = assigned_registers[jumpr.reg];
                               result.push_back(instruction::Line{instruction::Jumpr{mapped_reg}});
                           },
                           [&](const Label &label) {},
                           [&](const Halt &) { result.push_back(instruction::Line{instruction::Halt{}}); },
                       },
                       instruction);
        }
    }
    return result;
}

void LirEmitter::change_vreg(VirtualInstruction &instruction, VirtualRegister new_vreg) {
    std::visit(overloaded{
                   [&](Read &) {},
                   [&](Write &) {},
                   [&](Load &load) { load.address = new_vreg; },
                   [&](Store &store) { store.address = new_vreg; },
                   [&](Add &add) { add.address = new_vreg; },
                   [&](Sub &sub) { sub.address = new_vreg; },
                   [&](Get &get) { get.address = new_vreg; },
                   [&](Put &put) { put.address = new_vreg; },
                   [&](Rst &rst) { rst.address = new_vreg; },
                   [&](Inc &inc) { inc.address = new_vreg; },
                   [&](Dec &dec) { dec.address = new_vreg; },
                   [&](Shl &shl) { shl.address = new_vreg; },
                   [&](Shr &shr) { shr.address = new_vreg; },
                   [&](Jump &) {},
                   [&](Jpos &) {},
                   [&](Jzero &) {},
                   [&](Strk &strk) { strk.reg = new_vreg; },
                   [&](Jumpr &jumpr) { jumpr.reg = new_vreg; },
                   [&](Label &) {},
                   [&](Halt &) {},
               },
               instruction);
}

void LirEmitter::allocate_memory(const std::string &name, uint64_t size) {
    memory_locations[name] = next_memory_location;
    next_memory_location += size;
}

auto LirEmitter::get_new_memory_location() -> uint64_t { return next_memory_location++; }

} // namespace lir
