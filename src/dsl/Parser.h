#pragma once

#include "dsl/Token.h"

#include <QVector>
#include <memory>
#include <variant>
#include <string>

namespace fininsight::dsl {

// —— AST 节点定义 ——

/// 前向声明
struct BinaryExpr;
struct NumberExpr;
struct IdentifierExpr;
struct FunctionExpr;

/// AST 节点 (std::variant)
using AstNode = std::variant<BinaryExpr, NumberExpr, IdentifierExpr, FunctionExpr>;

/// 二元运算节点
struct BinaryExpr {
    enum Op { GT, LT, GTE, LTE, EQ, NEQ, AND, OR };
    Op op;
    std::unique_ptr<AstNode> left;
    std::unique_ptr<AstNode> right;
};

/// 数值节点
struct NumberExpr {
    double value;
};

/// 标识符节点（MACD, RSI, Close ...）
struct IdentifierExpr {
    std::string name;
};

/// 函数调用节点 MA(20), SMA(5)
struct FunctionExpr {
    std::string name;
    int arg;
};

/// 递归下降解析器
class Parser {
public:
    explicit Parser(const QVector<Token>& tokens);

    AstNode parse();

    const QString& error() const { return error_; }
    bool hasError() const { return !error_.isEmpty(); }

private:
    AstNode expression();
    AstNode orExpr();
    AstNode andExpr();
    AstNode cmpExpr();
    AstNode atom();

    Token peek() const;
    Token advance();
    bool match(TokenType t);

    const QVector<Token>& tokens_;
    int pos_ = 0;
    QString error_;
};

} // namespace fininsight::dsl
