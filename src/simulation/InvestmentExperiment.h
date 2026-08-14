#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fininsight::simulation {

struct PricePoint {
    std::int64_t timestampMs = 0;
    double price = 0.0;
};

struct InvestmentExperimentRequest {
    std::string symbol;
    double initialCash = 0.0;
    std::int64_t startTimestampMs = 0;
    std::int64_t endTimestampMs = 0;
    double buyFee = 0.0;
};

enum class InvestmentExperimentError {
    None,
    InvalidSymbol,
    InvalidInitialCash,
    InvalidTimeRange,
    InvalidFee,
    EmptyPriceSeries,
    InvalidPricePoint,
    PriceSeriesNotStrictlyIncreasing,
    NoPriceInRange,
    InsufficientCash,
    QuantityOverflow,
    TradeRejected,
    ValuationFailed,
};

struct InvestmentExperimentResult {
    InvestmentExperimentError error = InvestmentExperimentError::None;
    std::string message;
    std::string symbol;
    std::int64_t executedTimestampMs = 0;
    double executedPrice = 0.0;
    std::int64_t endingTimestampMs = 0;
    double endingPrice = 0.0;
    std::int64_t quantity = 0;
    double endingCash = 0.0;
    double endingMarketValue = 0.0;
    double endingEquity = 0.0;
    double totalPnl = 0.0;
    double returnRate = 0.0;
    double maxDrawdown = 0.0;

    bool ok() const { return error == InvestmentExperimentError::None; }
};

InvestmentExperimentResult runBuyAndHoldExperiment(
    const InvestmentExperimentRequest& request,
    const std::vector<PricePoint>& prices);

std::string investmentExperimentErrorMessage(InvestmentExperimentError error);

} // namespace fininsight::simulation
