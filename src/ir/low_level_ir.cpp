#include "low_level_ir.hpp"
#include "common.hpp"

namespace lir {
auto to_string(const VirtualInstruction &instr) -> std::string {
    return std::visit(overloaded{
                          [&](const Read &) { return std::format("\tRead"); },
                          [&](const Write &) { return std::format("\tWrite"); },
                          [&](const Load & load) { return std::format("\tLoad %{}", load.address); },
                          [&](const Store & store) { return std::format("\tStore %{}", store.address); },
                          [&](const Add & add) { return std::format("\tAdd %{}", add.address); },
                          [&](const Sub & sub) { return std::format("\tSub %{}", sub.address); },
                          [&](const Get & get) { return std::format("\tGet %{}", get.address); },
                          [&](const Put & put) { return std::format("\tPut %{}", put.address); },
                          [&](const Rst & rst) { return std::format("\tRst %{}", rst.address); },
                          [&](const Inc & inc) { return std::format("\tInc %{}", inc.address); },
                          [&](const Dec & dec) { return std::format("\tDec %{}", dec.address); },
                          [&](const Shl & shl) { return std::format("\tShl %{}", shl.address); },
                          [&](const Shr & shr) { return std::format("\tShr %{}", shr.address); },
                          [&](const Jump &jump) { return std::format("\tJump '{}", jump.label); },
                          [&](const Jpos &jpos) { return std::format("\tJpos '{}", jpos.label); },
                          [&](const Jzero &jzero) { return std::format("\tJzero '{}", jzero.label); },
                          [&](const Strk & strk) { return std::format("\tStrk %{}", strk.reg); },
                          [&](const Jumpr & jumpr) { return std::format("\tJumpr %{}", jumpr.reg); },
                          [&](const Label &label) { return std::format("{}:", label.name); },
                          [&](const Halt &) { return std::format("\tHalt"); }},
                      instr);
}
} // namespace lir
