#pragma once

#include "dsl/Token.h"

#include <QString>
#include <QVector>

namespace fininsight::dsl {

/**
 * @brief 词法分析器 — 将输入字符串拆分为 Token 序列
 */
class Lexer {
public:
    QVector<Token> tokenize(const QString& input);

private:
    void skipWhitespace();
    Token readNumber();
    Token readIdentifier();

    const QChar* pos_ = nullptr;
    const QChar* end_ = nullptr;
};

} // namespace fininsight::dsl
