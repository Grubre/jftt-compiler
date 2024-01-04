#pragma once

#include "token.hpp"
#include <memory>
#include <variant>

struct PIdentifier {
    std::string name;
    // TODO: Change it to Identifier, add support for
    // pidentifier[pidnetifier], pidentifier[num]
};

struct Value {
    std::variant<uint64_t, std::string> value;

    Value(uint64_t num) : value(num) {}
    Value(std::string identifier) : value(std::move(identifier)) {}
};

struct Expression {
    Value left;
    Value right;
};

struct Condition {
    Value left;
    Value right;
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

enum class CommandType { Assignment };

struct Assignment {
    Token identifier;
    Expression expression;
};

struct Command {
    CommandType type;

    Command(Assignment assignment)
        : type(CommandType::Assignment), assignment(std::move(assignment)) {}

    Command(const Command &other) : type(other.type) {
        switch (type) {
        case CommandType::Assignment:
            new (&assignment) Assignment(other.assignment);
            break;
        }
    }

    union {
        Assignment assignment;
    };

    ~Command() {
        switch (type) {
        case CommandType::Assignment:
            assignment.~Assignment();
            break;
        }
    }
};

struct Declaration {};
