#pragma once

#include "simulation/Ledger.h"

#include <cstdint>
#include <string>
#include <vector>

namespace fininsight::analysis {

struct EvidenceSnapshot {
    std::uint32_t schemaVersion = 1;
    std::string source;
    std::string priceBasis;
    std::vector<simulation::Trade> trades;
    simulation::PortfolioSnapshot portfolio;
    std::int64_t startTimestampMs = 0;
    std::int64_t endTimestampMs = 0;
    double maxDrawdown = 0.0;
    std::int64_t drawdownStartTimestampMs = 0;
    std::int64_t drawdownEndTimestampMs = 0;
};

enum class FindingSeverity { Info, Warning };

struct BehaviorFinding {
    std::string code;
    FindingSeverity severity = FindingSeverity::Info;
    std::string message;
    double measure = 0.0;
    std::int64_t evidenceStartTimestampMs = 0;
    std::int64_t evidenceEndTimestampMs = 0;
    std::vector<std::uint64_t> evidenceTradeIds;
};

struct BehaviorReport {
    std::size_t tradeCount = 0;
    std::size_t buyCount = 0;
    std::size_t sellCount = 0;
    std::size_t symbolCount = 0;
    double tradesPerDay = 0.0;
    double buyNotional = 0.0;
    double maxSymbolConcentration = 0.0;
    double winningSellRate = 0.0;
    double profitFactor = 0.0;
    double averageWin = 0.0;
    double averageLoss = 0.0;
    std::size_t tradesDuringDrawdown = 0;
    std::vector<BehaviorFinding> findings;
};

BehaviorReport analyzeBehavior(const EvidenceSnapshot& evidence);

} // namespace fininsight::analysis
