#include "dsl/Lexer.h"

namespace fininsight::dsl {

QVector<Token> Lexer::tokenize(const QString& input) {
    QVector<Token> result;
    pos_ = input.data();
    end_ = pos_ + input.size();

    while (pos_ < end_) {
        skipWhitespace();
        if (pos_ >= end_) break;

        QChar c = *pos_;

        if (c.isDigit()) {
            result.append(readNumber());
        }
        else if (c.isLetter()) {
            result.append(readIdentifier());
        }
        else if (c == '>') {
            ++pos_;
            if (pos_ < end_ && *pos_ == '=') {
                result.append({TokenType::Gte, ">="});
                ++pos_;
            } else {
                result.append({TokenType::Gt, ">"});
            }
        }
        else if (c == '<') {
            ++pos_;
            if (pos_ < end_ && *pos_ == '=') {
                result.append({TokenType::Lte, "<="});
                ++pos_;
            } else {
                result.append({TokenType::Lt, "<"});
            }
        }
        else if (c == '=') {
            ++pos_;
            if (pos_ < end_ && *pos_ == '=') {
                result.append({TokenType::Eq, "=="});
                ++pos_;
            }
        }
        else if (c == '!') {
            ++pos_;
            if (pos_ < end_ && *pos_ == '=') {
                result.append({TokenType::Neq, "!="});
                ++pos_;
            }
        }
        else if (c == '(') {
            result.append({TokenType::LParen, "("});
            ++pos_;
        }
        else if (c == ')') {
            result.append({TokenType::RParen, ")"});
            ++pos_;
        }
        else {
            qWarning() << "[Lexer] Unexpected char:" << c;
            ++pos_;
        }
    }

    result.append({TokenType::Eof, ""});
    return result;
}

void Lexer::skipWhitespace() {
    while (pos_ < end_ && pos_->isSpace()) ++pos_;
}

Token Lexer::readNumber() {
    QString num;
    while (pos_ < end_ && (pos_->isDigit() || *pos_ == '.')) {
        num += *pos_;
        ++pos_;
    }
    return {TokenType::Number, num};
}

Token Lexer::readIdentifier() {
    QString id;
    while (pos_ < end_ && (pos_->isLetterOrNumber() || *pos_ == '_')) {
        id += *pos_;
        ++pos_;
    }

    // 关键字识别
    QString upper = id.toUpper();
    if (upper == "AND") return {TokenType::And, id};
    if (upper == "OR")  return {TokenType::Or, id};

    return {TokenType::Identifier, id};
}

} // namespace fininsight::dsl
