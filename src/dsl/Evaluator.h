#pragma once

#include "dsl/Parser.h"

#include <vector>
#include <functional>
#include <QString>

namespace fininsight::dsl {

/**
 * @brief AST 求值器 — 遍历 AST，结合市场数据计算表达式结果
 *
 * 支持的标识符：
 *   Close, Open, High, Low, Volume  → 最新一根 K 线的值
 *   MA(n), SMA(n)                    → 简单移动平均
 *   EMA(n)                           → 指数移动平均
 *   MACD                             → MACD DIF 线最新值
 *   Signal                           → MACD Signal 线最新值
 *   RSI(n)                           → RSI 最新值（默认 14）
 *   BOLL_U, BOLL_M, BOLL_L           → 布林带上中下轨
 */
class Evaluator {
public:
    /// 设置评估所需的 K 线数据
    void setData(const std::vector<double>& opens,
                 const std::vector<double>& highs,
                 const std::vector<double>& lows,
                 const std::vector<double>& closes,
                 const std::vector<double>& volumes);

    /// 求值
    double evaluate(const AstNode& node);

    /// 便捷方法：解析 + 求值一步完成
    /// @return 表达式结果，若解析失败则返回 error
    struct Result {
        bool ok;
        double value;
        QString error;
    };
    Result evaluate(const QString& expression);

private:
    double visit(const BinaryExpr& e);
    double visit(const NumberExpr& e);
    double visit(const IdentifierExpr& e);
    double visit(const FunctionExpr& e);

    double getLatest(const std::vector<double>& data) const;
    double getMACDLine() const;
    double getMACDSignal() const;

    std::vector<double> opens_, highs_, lows_, closes_, volumes_;
};

} // namespace fininsight::dsl
