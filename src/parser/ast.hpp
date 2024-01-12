#pragma once

#include "token.hpp"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace parser {

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

} // namespace parser
