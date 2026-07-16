#include "dsl/Parser.h"
#include <stdexcept>

namespace fininsight::dsl {

Parser::Parser(const QVector<Token>& tokens) : tokens_(tokens) {}

AstNode Parser::parse() {
    return expression();
}

Token Parser::peek() const {
    return tokens_[pos_];
}

Token Parser::advance() {
    return tokens_[pos_++];
}

bool Parser::match(TokenType t) {
    if (peek().type == t) { advance(); return true; }
    return false;
}

// ── 表达式 (OR 最低优先级) ─────────────────────────

AstNode Parser::expression() {
    return orExpr();
}

AstNode Parser::orExpr() {
    auto left = andExpr();
    while (peek().type == TokenType::Or) {
        advance();
        auto right = andExpr();
        left = BinaryExpr{BinaryExpr::OR,
            std::make_unique<AstNode>(std::move(left)),
            std::make_unique<AstNode>(std::move(right))};
    }
    return left;
}

AstNode Parser::andExpr() {
    auto left = cmpExpr();
    while (peek().type == TokenType::And) {
        advance();
        auto right = cmpExpr();
        left = BinaryExpr{BinaryExpr::AND,
            std::make_unique<AstNode>(std::move(left)),
            std::make_unique<AstNode>(std::move(right))};
    }
    return left;
}

AstNode Parser::cmpExpr() {
    auto left = atom();
    auto t = peek();
    if (t.type == TokenType::Gt || t.type == TokenType::Lt ||
        t.type == TokenType::Gte || t.type == TokenType::Lte ||
        t.type == TokenType::Eq || t.type == TokenType::Neq)
    {
        advance();
        auto right = atom();
        BinaryExpr::Op op;
        switch (t.type) {
        case TokenType::Gt:  op = BinaryExpr::GT;  break;
        case TokenType::Lt:  op = BinaryExpr::LT;  break;
        case TokenType::Gte: op = BinaryExpr::GTE; break;
        case TokenType::Lte: op = BinaryExpr::LTE; break;
        case TokenType::Eq:  op = BinaryExpr::EQ;  break;
        case TokenType::Neq: op = BinaryExpr::NEQ; break;
        default: op = BinaryExpr::GT;
        }
        left = BinaryExpr{op,
            std::make_unique<AstNode>(std::move(left)),
            std::make_unique<AstNode>(std::move(right))};
    }
    return left;
}

AstNode Parser::atom() {
    if (match(TokenType::LParen)) {
        auto expr = expression();
        if (!match(TokenType::RParen)) {
            error_ = "Expected ')'";
        }
        return expr;
    }

    if (match(TokenType::Number)) {
        return NumberExpr{static_cast<double>(tokens_[pos_-1].text.toInt())};
    }

    if (peek().type == TokenType::Identifier) {
        auto idToken = advance();

        // 函数调用: MA(20)
        if (peek().type == TokenType::LParen) {
            advance(); // '('
            if (peek().type != TokenType::Number) {
                error_ = "Expected number inside function call";
                return IdentifierExpr{idToken.text.toStdString()};
            }
            auto numToken = advance();
            match(TokenType::RParen);
            return FunctionExpr{idToken.text.toStdString(), numToken.text.toInt()};
        }

        return IdentifierExpr{idToken.text.toStdString()};
    }

    error_ = "Unexpected token: " + peek().toString();
    return IdentifierExpr{"ERROR"};
}

} // namespace fininsight::dsl
