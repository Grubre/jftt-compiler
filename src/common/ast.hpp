#pragma once

#include "token.hpp"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace ast {

struct Identifier {
    Token name;
    std::optional<Token> index;

    auto get_str() const -> std::string const {
        auto result = name.lexeme;
        if (index) {
            result += "[";
            result += index->lexeme;
            result += "]";
        }
        return result;
    }
};

using Num = Token;
using Value = std::variant<Num, Identifier>;

inline auto get_str(const Value &value) -> std::string {
    if (std::holds_alternative<Num>(value)) {
        return std::get<Num>(value).lexeme;
    }
    return std::get<Identifier>(value).get_str();
}

struct BinaryExpression {
    Value lhs;
    Token op;
    Value rhs;
};

using Expression = std::variant<BinaryExpression, Value>;

struct Condition {
    Value lhs;
    Token op;
    Value rhs;
};

enum class NodeType {
    Expression,
    Num,
    Pidentifier,
};

enum class CommandType { Assignment, Read };

struct Assignment {
    Identifier identifier;
    Expression expression;
};

struct Read {
    Identifier identifier;
};

struct Write {
    Value value;
};

struct Call {
    Token name;
    std::vector<Token> args;
};

struct If;
struct Repeat;
struct While;

using Command = std::variant<Assignment, Read, Write, While, Call, If, Repeat>;

struct While {
    Condition condition;
    std::vector<Command> commands;
};

struct If {
    Condition condition;
    std::vector<Command> commands;
    std::optional<std::vector<Command>> else_commands;
};

struct Repeat {
    std::vector<Command> commands;
    Condition condition;
};

struct Declaration {
    Token identifier;
    std::optional<Token> array_size;
};

struct Context {
    std::vector<Declaration> declarations{};
    std::vector<Command> commands{};
};

struct Arg {
    Token identifier;
    bool is_array;
};

struct Procedure {
    Token name;
    std::vector<Arg> args{};
    Context context;

    std::string signature() const {
        std::string result = name.lexeme + "(";

        for (const auto &arg : args) {
            result += arg.identifier.lexeme + ", ";
        }

        if (!args.empty()) {
            result.pop_back();
            result.pop_back();
        }

        return result + ")";
    }
};

struct Program {
    std::vector<Procedure> procedures{};
    Context main;
};


} // namespace parser