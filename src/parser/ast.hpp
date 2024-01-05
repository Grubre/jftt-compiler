#pragma once

#include "token.hpp"
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace parser {

struct Identifier {
    // TODO: The name field can only be an PIdentifier
    // The index field can only be a Num or PIdentifier
    // but this is not enforced by the type system here
    Token name;
    std::optional<Token> index;
};

using Num = Token;
using Value = std::variant<Num, Identifier>;

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

// struct AstNode {
//     NodeType type;
//
//     AstNode(Expression expr)
//         : type(NodeType::Expression), expression(std::move(expr)) {}
//     AstNode(uint64_t num) : num(num) {}
//     AstNode(PIdentifier pidentifier)
//         : type(NodeType::Pidentifier), pidentifier(std::move(pidentifier)) {}
//
//     union {
//         Expression expression;
//         uint64_t num;
//         PIdentifier pidentifier;
//     };
//
//     ~AstNode() {
//         switch (type) {
//         case NodeType::Expression:
//             expression.~Expression();
//             break;
//         case NodeType::Num:
//             break;
//         case NodeType::Pidentifier:
//             pidentifier.~PIdentifier();
//             break;
//         }
//     }
// };

enum class CommandType { Assignment, Read };

struct Assignment {
    Identifier identifier;
    Expression expression;
};

struct Read {
    Token identifier;
};

struct Write {
    Value value;
};

struct Call {
    Token name;
    std::vector<Token> args;
};

struct While;

// NOTE: Possibly need to use pointer for While
using Command = std::variant<Assignment, Read, Write, While, Call>;

struct While {
    Condition condition;
    std::vector<Command> commands;
};

// struct Command {
//     CommandType type;
//
//     Command(Assignment assignment)
//         : type(CommandType::Assignment), assignment(std::move(assignment)) {}
//
//     Command(const Command &other) : type(other.type) {
//         switch (type) {
//         case CommandType::Assignment:
//             new (&assignment) Assignment(other.assignment);
//             break;
//         case CommandType::Read:
//             break;
//         }
//     }
//
//     Command(Command &&other) : type(other.type) {
//         switch (type) {
//         case CommandType::Assignment:
//             new (&assignment) Assignment(std::move(other.assignment));
//             break;
//         case CommandType::Read:
//             new (&read) Read(std::move(other.read));
//             break;
//         }
//     }
//
//     auto operator=(const Command &other) -> Command & {
//         if (this == &other) {
//             return *this;
//         }
//
//         this->~Command();
//         new (this) Command(other);
//         return *this;
//     }
//
//     auto operator=(Command &&other) -> Command & {
//         if (this == &other) {
//             return *this;
//         }
//
//         this->~Command();
//         new (this) Command(std::move(other));
//         return *this;
//     }
//
//     union {
//         Assignment assignment;
//         Read read;
//     };
//
//     ~Command() {
//         switch (type) {
//         case CommandType::Assignment:
//             assignment.~Assignment();
//             break;
//         case CommandType::Read:
//             read.~Read();
//             break;
//         }
//     }
// };

struct Declaration {
    Token identifier;
    std::optional<Token> array_size;
};

} // namespace parser
