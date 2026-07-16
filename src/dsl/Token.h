#pragma once

#include <QString>
#include <QDebug>

namespace fininsight::dsl {

/// Token 类型枚举
enum class TokenType {
    Identifier,   // MACD, Signal, RSI, Close, MA ...
    Number,       // 20, 30, 14 ...
    Gt,           // >
    Lt,           // <
    Gte,          // >=
    Lte,          // <=
    Eq,           // ==
    Neq,          // !=
    And,          // AND
    Or,           // OR
    LParen,       // (
    RParen,       // )
    Eof           // 结束标记
};

/// Token 结构体
struct Token {
    TokenType type;
    QString   text;   // 原始文本（Identifier/Number 时有用）

    Token(TokenType t, QString s = {}) : type(t), text(std::move(s)) {}

    QString toString() const {
        switch (type) {
        case TokenType::Identifier: return "ID(" + text + ")";
        case TokenType::Number:     return "NUM(" + text + ")";
        case TokenType::Gt:   return ">";
        case TokenType::Lt:   return "<";
        case TokenType::Gte:  return ">=";
        case TokenType::Lte:  return "<=";
        case TokenType::Eq:   return "==";
        case TokenType::Neq:  return "!=";
        case TokenType::And:  return "AND";
        case TokenType::Or:   return "OR";
        case TokenType::LParen: return "(";
        case TokenType::RParen: return ")";
        case TokenType::Eof:  return "EOF";
        }
        return "?";
    }
};

} // namespace fininsight::dsl
