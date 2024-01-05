#pragma once

#include <cstdint>
#include <variant>

namespace emitter {

enum class Register { A, B, C, D, E, F, G, H };

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

} // namespace emitter
