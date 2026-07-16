#include "dsl/Evaluator.h"
#include "dsl/Lexer.h"
#include "charts/IndicatorEngine.h"

namespace fininsight::dsl {
using namespace fininsight::charts;

// ── 设置数据 ────────────────────────────────────────

void Evaluator::setData(const std::vector<double>& opens,
                         const std::vector<double>& highs,
                         const std::vector<double>& lows,
                         const std::vector<double>& closes,
                         const std::vector<double>& volumes) {
    opens_   = opens;
    highs_   = highs;
    lows_    = lows;
    closes_  = closes;
    volumes_ = volumes;
}

// ── 入口 ────────────────────────────────────────────

double Evaluator::evaluate(const AstNode& node) {
    return std::visit([this](const auto& e) { return this->visit(e); }, node);
}

Evaluator::Result Evaluator::evaluate(const QString& expression) {
    Lexer lexer;
    auto tokens = lexer.tokenize(expression);

    Parser parser(tokens);
    auto ast = parser.parse();

    if (parser.hasError()) {
        return {false, 0.0, parser.error()};
    }

    double val = evaluate(ast);
    return {true, val, {}};
}

// ── visit 重载 ──────────────────────────────────────

double Evaluator::visit(const BinaryExpr& e) {
    double l = std::visit([this](const auto& n) { return this->visit(n); }, *e.left);
    double r = std::visit([this](const auto& n) { return this->visit(n); }, *e.right);

    switch (e.op) {
    case BinaryExpr::GT:  return l > r ? 1.0 : 0.0;
    case BinaryExpr::LT:  return l < r ? 1.0 : 0.0;
    case BinaryExpr::GTE: return l >= r ? 1.0 : 0.0;
    case BinaryExpr::LTE: return l <= r ? 1.0 : 0.0;
    case BinaryExpr::EQ:  return l == r ? 1.0 : 0.0;
    case BinaryExpr::NEQ: return l != r ? 1.0 : 0.0;
    case BinaryExpr::AND: return (l > 0.5 && r > 0.5) ? 1.0 : 0.0;
    case BinaryExpr::OR:  return (l > 0.5 || r > 0.5) ? 1.0 : 0.0;
    }
    return 0.0;
}

double Evaluator::visit(const NumberExpr& e) {
    return e.value;
}

double Evaluator::visit(const IdentifierExpr& e) {
    const auto& name = e.name;

    if (name == "Close")  return getLatest(closes_);
    if (name == "Open")   return getLatest(opens_);
    if (name == "High")   return getLatest(highs_);
    if (name == "Low")    return getLatest(lows_);
    if (name == "Volume") return getLatest(volumes_);
    if (name == "MACD")   return getMACDLine();
    if (name == "Signal") return getMACDSignal();
    if (name == "RSI") {
        if (closes_.size() < 14) return 50.0;
        auto rsi = computeRSI(closes_, 14);
        return getLatest(rsi);
    }
    if (name == "BOLL_U") {
        auto b = computeBollinger(closes_, 20, 2.0);
        return getLatest(b.upper);
    }
    if (name == "BOLL_M") {
        auto b = computeBollinger(closes_, 20, 2.0);
        return getLatest(b.middle);
    }
    if (name == "BOLL_L") {
        auto b = computeBollinger(closes_, 20, 2.0);
        return getLatest(b.lower);
    }

    return 0.0; // 未知标识符
}

double Evaluator::visit(const FunctionExpr& e) {
    const auto& name = e.name;
    int n = e.arg;

    if ((name == "MA" || name == "SMA") && !closes_.empty()) {
        auto sma = computeSMA(closes_, n);
        return getLatest(sma);
    }
    if (name == "EMA" && !closes_.empty()) {
        auto ema = computeEMA(closes_, n);
        return getLatest(ema);
    }
    if (name == "RSI" && closes_.size() >= static_cast<size_t>(n)) {
        auto rsi = computeRSI(closes_, n);
        return getLatest(rsi);
    }

    return 0.0;
}

// ── 辅助 ────────────────────────────────────────────

double Evaluator::getLatest(const std::vector<double>& data) const {
    if (data.empty()) return 0.0;
    return data.back();
}

double Evaluator::getMACDLine() const {
    if (closes_.size() < 26) return 0.0;
    auto macd = computeMACD(closes_);
    return getLatest(macd.line);
}

double Evaluator::getMACDSignal() const {
    if (closes_.size() < 26) return 0.0;
    auto macd = computeMACD(closes_);
    return getLatest(macd.signal);
}

} // namespace fininsight::dsl
