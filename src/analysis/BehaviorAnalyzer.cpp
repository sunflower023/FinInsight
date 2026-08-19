#include "analysis/BehaviorAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace fininsight::analysis {

BehaviorReport analyzeBehavior(const EvidenceSnapshot& evidence) {
    BehaviorReport report;
    report.tradeCount = evidence.trades.size();
    std::unordered_map<std::string, double> notionalBySymbol;
    std::unordered_map<std::string, std::vector<std::uint64_t>> tradeIdsBySymbol;
    std::vector<std::uint64_t> sellTradeIds;
    std::vector<std::uint64_t> drawdownTradeIds;
    double grossProfit = 0.0;
    double grossLoss = 0.0;
    std::size_t wins = 0;
    std::size_t losses = 0;

    for (const auto& trade : evidence.trades) {
        if (trade.side == simulation::TradeSide::Buy) {
            ++report.buyCount;
            const double notional = trade.price * static_cast<double>(trade.quantity) + trade.fee;
            if (std::isfinite(notional) && notional > 0.0) {
                report.buyNotional += notional;
                notionalBySymbol[trade.symbol] += notional;
                tradeIdsBySymbol[trade.symbol].push_back(trade.id);
            }
        } else {
            ++report.sellCount;
            sellTradeIds.push_back(trade.id);
            if (trade.realizedPnl > 0.0) {
                grossProfit += trade.realizedPnl;
                ++wins;
            } else if (trade.realizedPnl < 0.0) {
                grossLoss += -trade.realizedPnl;
                ++losses;
            }
        }
        if (evidence.drawdownStartTimestampMs > 0 && evidence.drawdownEndTimestampMs >= evidence.drawdownStartTimestampMs &&
            trade.timestampMs >= evidence.drawdownStartTimestampMs && trade.timestampMs <= evidence.drawdownEndTimestampMs) {
            ++report.tradesDuringDrawdown;
            drawdownTradeIds.push_back(trade.id);
        }
    }

    report.symbolCount = notionalBySymbol.size();
    if (report.buyNotional > 0.0) {
        for (const auto& [symbol, notional] : notionalBySymbol) {
            report.maxSymbolConcentration = std::max(report.maxSymbolConcentration, notional / report.buyNotional);
        }
    }
    const auto durationMs = evidence.endTimestampMs > evidence.startTimestampMs
        ? evidence.endTimestampMs - evidence.startTimestampMs : 0;
    if (durationMs > 0) report.tradesPerDay = static_cast<double>(report.tradeCount) / (durationMs / 86400000.0);
    report.winningSellRate = report.sellCount > 0 ? static_cast<double>(wins) / report.sellCount : 0.0;
    report.profitFactor = grossLoss > 0.0 ? grossProfit / grossLoss : (grossProfit > 0.0 ? std::numeric_limits<double>::infinity() : 0.0);
    report.averageWin = wins > 0 ? grossProfit / static_cast<double>(wins) : 0.0;
    report.averageLoss = losses > 0 ? grossLoss / static_cast<double>(losses) : 0.0;

    if (report.tradeCount >= 10 || report.tradesPerDay >= 2.0) {
        report.findings.push_back({"FREQUENT_TRADING", FindingSeverity::Warning,
                                   "Trading frequency is elevated; review the evidence for each trade.", report.tradesPerDay});
    }
    std::string concentratedSymbol;
    if (report.buyNotional > 0.0) {
        for (const auto& [symbol, notional] : notionalBySymbol) {
            if (notional / report.buyNotional > report.maxSymbolConcentration) {
                concentratedSymbol = symbol;
            }
        }
    }
    if (report.maxSymbolConcentration >= 0.5 && report.symbolCount > 0) {
        report.findings.push_back({"CONCENTRATION", FindingSeverity::Warning,
                                   "Buy notional is concentrated in a small number of symbols.", report.maxSymbolConcentration});
    }
    if (report.averageLoss > 0.0 && report.averageWin > 0.0 && report.averageLoss > report.averageWin * 1.5) {
        report.findings.push_back({"LOSS_ASYMMETRY", FindingSeverity::Warning,
                                   "Average losses are materially larger than average wins; review exit rules.", report.averageLoss / report.averageWin});
    }
    if (evidence.maxDrawdown >= 0.10 && report.tradesDuringDrawdown > 0) {
        report.findings.push_back({"TRADING_DURING_DRAWDOWN", FindingSeverity::Info,
                                   "Trades occurred during the maximum drawdown window; review the supporting evidence.", evidence.maxDrawdown});
    }
    for (auto& finding : report.findings) {
        finding.evidenceStartTimestampMs = evidence.startTimestampMs;
        finding.evidenceEndTimestampMs = evidence.endTimestampMs;
        if (finding.code == "CONCENTRATION") {
            finding.evidenceTradeIds = tradeIdsBySymbol[concentratedSymbol];
        } else if (finding.code == "LOSS_ASYMMETRY") {
            finding.evidenceTradeIds = sellTradeIds;
        } else if (finding.code == "TRADING_DURING_DRAWDOWN") {
            finding.evidenceTradeIds = drawdownTradeIds;
        } else {
            for (const auto& trade : evidence.trades) finding.evidenceTradeIds.push_back(trade.id);
        }
    }
    return report;
}

} // namespace fininsight::analysis
