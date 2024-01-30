#include "low_level_ir.hpp"
#include "common.hpp"

namespace lir {
auto to_string(const VirtualInstruction &instr) -> std::string {
    return std::visit(overloaded{[&](const Read &) { return std::format("\tRead"); },
                                 [&](const Write &) { return std::format("\tWrite"); },
                                 [&](const Load &load) { return std::format("\tLoad %{}", load.address); },
                                 [&](const Store &store) { return std::format("\tStore %{}", store.address); },
                                 [&](const Add &add) { return std::format("\tAdd %{}", add.address); },
                                 [&](const Sub &sub) { return std::format("\tSub %{}", sub.address); },
                                 [&](const Get &get) { return std::format("\tGet %{}", get.address); },
                                 [&](const Put &put) { return std::format("\tPut %{}", put.address); },
                                 [&](const Rst &rst) { return std::format("\tRst %{}", rst.address); },
                                 [&](const Inc &inc) { return std::format("\tInc %{}", inc.address); },
                                 [&](const Dec &dec) { return std::format("\tDec %{}", dec.address); },
                                 [&](const Shl &shl) { return std::format("\tShl %{}", shl.address); },
                                 [&](const Shr &shr) { return std::format("\tShr %{}", shr.address); },
                                 [&](const Jump &jump) { return std::format("\tJump '{}", jump.label); },
                                 [&](const Jpos &jpos) { return std::format("\tJpos '{}", jpos.label); },
                                 [&](const Jzero &jzero) { return std::format("\tJzero '{}", jzero.label); },
                                 [&](const Strk &strk) { return std::format("\tStrk %{}", strk.reg); },
                                 [&](const Jumpr &jumpr) { return std::format("\tJumpr %{}", jumpr.reg); },
                                 [&](const Label &label) { return std::format("{}:", label.name); },
                                 [&](const Halt &) { return std::format("\tHalt"); }},
                      instr);
}

auto read_variables(const VirtualInstruction &instr) -> std::vector<VirtualRegister> {
    return std::visit(overloaded{[&](const Read &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Write &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Load &load) { return std::vector<VirtualRegister>{load.address}; },
                                 [&](const Store &store) {
                                     return std::vector<VirtualRegister>{regA, store.address};
                                 },
                                 [&](const Add &add) {
                                     return std::vector<VirtualRegister>{regA, add.address};
                                 },
                                 [&](const Sub &sub) {
                                     return std::vector<VirtualRegister>{regA, sub.address};
                                 },
                                 [&](const Get &get) { return std::vector<VirtualRegister>{get.address}; },
                                 [&](const Put &put) { return std::vector<VirtualRegister>{regA}; },
                                 [&](const Rst &rst) { return std::vector<VirtualRegister>{}; },
                                 [&](const Inc &inc) { return std::vector<VirtualRegister>{inc.address}; },
                                 [&](const Dec &dec) { return std::vector<VirtualRegister>{dec.address}; },
                                 [&](const Shl &shl) { return std::vector<VirtualRegister>{shl.address}; },
                                 [&](const Shr &shr) { return std::vector<VirtualRegister>{shr.address}; },
                                 [&](const Jump &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Jpos &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Jzero &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Strk &strk) { return std::vector<VirtualRegister>{}; },
                                 [&](const Jumpr &jumpr) { return std::vector<VirtualRegister>{jumpr.reg}; },
                                 [&](const Label &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Halt &) { return std::vector<VirtualRegister>{}; }},
                      instr);
}

auto overwritten_variables(const VirtualInstruction &instr) -> std::vector<VirtualRegister> {
    return std::visit(overloaded{[&](const Read &) { return std::vector<VirtualRegister>{regA}; },
                                 [&](const Write &) { return std::vector<VirtualRegister>{regA}; },
                                 [&](const Load &) { return std::vector<VirtualRegister>{regA}; },
                                 [&](const Store &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Add &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Sub &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Get &) { return std::vector<VirtualRegister>{regA}; },
                                 [&](const Put &put) { return std::vector<VirtualRegister>{put.address}; },
                                 [&](const Rst &rst) { return std::vector<VirtualRegister>{rst.address}; },
                                 [&](const Inc &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Dec &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Shl &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Shr &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Jump &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Jpos &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Jzero &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Strk &strk) { return std::vector<VirtualRegister>{strk.reg}; },
                                 [&](const Jumpr &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Label &) { return std::vector<VirtualRegister>{}; },
                                 [&](const Halt &) { return std::vector<VirtualRegister>{}; }},
                      instr);
}
} // namespace lir
