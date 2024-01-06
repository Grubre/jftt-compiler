#pragma once

#include <cstdint>
#include <string>
#include <variant>

template <class> inline constexpr bool always_false_v = false;

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace emitter {

enum class Register { A, B, C, D, E, F, G, H };

inline auto to_string(Register reg) -> std::string {
    switch (reg) {
    case Register::A:
        return "a";
    case Register::B:
        return "b";
    case Register::C:
        return "c";
    case Register::D:
        return "d";
    case Register::E:
        return "e";
    case Register::F:
        return "f";
    case Register::G:
        return "g";
    case Register::H:
        return "h";
    }
}

struct Read {};
struct Write {};

struct Load {
    Register address;
};
struct Store {
    Register address;
};
struct Add {
    Register address;
};
struct Sub {
    Register address;
};
struct Get {
    Register address;
};
struct Put {
    Register address;
};
struct Rst {
    Register address;
};
struct Inc {
    Register address;
};
struct Dec {
    Register address;
};
struct Shl {
    Register address;
};
struct Shr {
    Register address;
};

struct Jump {
    uint64_t line;
};
struct Jpos {
    uint64_t line;
};
struct Jzero {
    uint64_t line;
};

struct Strk {
    Register reg;
};
struct Jumpr {
    Register reg;
};

struct Halt {};

using Instruction =
    std::variant<Read, Write, Load, Store, Add, Sub, Get, Put, Rst, Inc, Dec,
                 Shl, Shr, Jump, Jpos, Jzero, Strk, Jumpr, Halt>;

inline auto to_string(const Instruction &instruction) -> std::string {
    return std::visit(overloaded{
                          [](const Read &) -> std::string { return "READ"; },
                          [](const Write &) -> std::string { return "WRITE"; },
                          [](const Load &load) -> std::string {
                              return "LOAD " + to_string(load.address);
                          },
                          [](const Store &store) -> std::string {
                              return "STORE " + to_string(store.address);
                          },
                          [](const Add &add) -> std::string {
                              return "ADD " + to_string(add.address);
                          },
                          [](const Sub &sub) -> std::string {
                              return "SUB " + to_string(sub.address);
                          },
                          [](const Get &get) -> std::string {
                              return "GET " + to_string(get.address);
                          },
                          [](const Put &put) -> std::string {
                              return "PUT " + to_string(put.address);
                          },
                          [](const Rst &rst) -> std::string {
                              return "RST " + to_string(rst.address);
                          },
                          [](const Inc &inc) -> std::string {
                              return "INC " + to_string(inc.address);
                          },
                          [](const Dec &dec) -> std::string {
                              return "DEC " + to_string(dec.address);
                          },
                          [](const Shl &shl) -> std::string {
                              return "SHL " + to_string(shl.address);
                          },
                          [](const Shr &shr) -> std::string {
                              return "SHR " + to_string(shr.address);
                          },
                          [](const Jump &jump) -> std::string {
                              return "JUMP " + std::to_string(jump.line);
                          },
                          [](const Jpos &jpos) -> std::string {
                              return "JPOS " + std::to_string(jpos.line);
                          },
                          [](const Jzero &jzero) -> std::string {
                              return "JZERO " + std::to_string(jzero.line);
                          },
                          [](const Strk &strk) -> std::string {
                              return "STRK " + to_string(strk.reg);
                          },
                          [](const Jumpr &jumpr) -> std::string {
                              return "JUMPR " + to_string(jumpr.reg);
                          },
                          [](const Halt &) -> std::string { return "HALT"; },
                      },
                      instruction);
}

struct Line {
    Instruction instruction;
    std::string comment{};
};

} // namespace emitter
