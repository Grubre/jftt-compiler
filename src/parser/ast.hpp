#pragma once

#include <memory>
struct AstNode;
using AstNodePtr = std::unique_ptr<AstNode>;

struct Expression {
    AstNodePtr left;
    AstNodePtr right;

    Expression(AstNodePtr left, AstNodePtr right)
        : left(std::move(left)), right(std::move(right)) {}
};

struct Num {
    uint64_t value;
};

struct PIdentifier {
    std::string name;
};

enum class NodeType {
    Expression,
    Num,
    Pidentifier,
};

struct AstNode {
    NodeType type;

    AstNode(Expression expr)
        : type(NodeType::Expression), expression(std::move(expr)) {}
    AstNode(Num num) : type(NodeType::Num), num(std::move(num)) {}
    AstNode(PIdentifier pidentifier)
        : type(NodeType::Pidentifier), pidentifier(std::move(pidentifier)) {}

    union {
        Expression expression;
        Num num;
        PIdentifier pidentifier;
    };
};
