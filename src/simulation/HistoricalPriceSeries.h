#pragma once

#include "simulation/InvestmentExperiment.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fininsight::simulation {

enum class HistoricalPriceField {
    Close,
    AdjustedClose,
};

struct HistoricalBar {
    std::string symbol;
    std::string tradingDate;
    double close = 0.0;
    std::optional<double> adjustedClose;
};

enum class HistoricalPriceSeriesError {
    None,
    InvalidExpectedSymbol,
    EmptyBars,
    InvalidBarSymbol,
    SymbolMismatch,
    InvalidTradingDate,
    TradingDatesNotStrictlyIncreasing,
    InvalidClose,
    MissingAdjustedClose,
    InvalidAdjustedClose,
};

struct HistoricalPriceSeriesResult {
    HistoricalPriceSeriesError error = HistoricalPriceSeriesError::None;
    std::string message;
    std::string symbol;
    HistoricalPriceField priceField = HistoricalPriceField::Close;
    std::vector<PricePoint> prices;

    bool ok() const { return error == HistoricalPriceSeriesError::None; }
};

// Trading dates are represented as UTC midnight solely to provide a stable,
// timezone-independent ordering key for daily bars.
std::optional<std::int64_t> tradingDateToTimestampMs(std::string_view tradingDate);

HistoricalPriceSeriesResult buildHistoricalPriceSeries(
    std::string_view expectedSymbol,
    const std::vector<HistoricalBar>& bars,
    HistoricalPriceField priceField = HistoricalPriceField::Close);

std::string historicalPriceSeriesErrorMessage(HistoricalPriceSeriesError error);

} // namespace fininsight::simulation
