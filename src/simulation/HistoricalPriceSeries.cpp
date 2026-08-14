#include "simulation/HistoricalPriceSeries.h"

#include "market/QuoteRules.h"

#include <chrono>
#include <cmath>
#include <utility>

namespace fininsight::simulation {
namespace {

bool parseDigits(std::string_view value, std::size_t offset, std::size_t count,
                 unsigned& result)
{
    result = 0;
    for (std::size_t index = offset; index < offset + count; ++index) {
        const char ch = value[index];
        if (ch < '0' || ch > '9') return false;
        result = result * 10U + static_cast<unsigned>(ch - '0');
    }
    return true;
}

HistoricalPriceSeriesResult failure(HistoricalPriceSeriesError error,
                                    std::string detail = {})
{
    HistoricalPriceSeriesResult result;
    result.error = error;
    result.message = detail.empty() ? historicalPriceSeriesErrorMessage(error)
                                    : std::move(detail);
    return result;
}

bool isValidPrice(double price)
{
    return std::isfinite(price) && price > 0.0;
}

} // namespace

std::optional<std::int64_t> tradingDateToTimestampMs(std::string_view tradingDate)
{
    if (tradingDate.size() != 10 || tradingDate[4] != '-' || tradingDate[7] != '-') {
        return std::nullopt;
    }

    unsigned yearValue = 0;
    unsigned monthValue = 0;
    unsigned dayValue = 0;
    if (!parseDigits(tradingDate, 0, 4, yearValue)
        || !parseDigits(tradingDate, 5, 2, monthValue)
        || !parseDigits(tradingDate, 8, 2, dayValue)) {
        return std::nullopt;
    }

    const std::chrono::year_month_day date{
        std::chrono::year{static_cast<int>(yearValue)},
        std::chrono::month{monthValue},
        std::chrono::day{dayValue},
    };
    if (!date.ok()) return std::nullopt;

    const auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::sys_days{date}.time_since_epoch()).count();
    if (timestampMs <= 0) return std::nullopt;
    return timestampMs;
}

HistoricalPriceSeriesResult buildHistoricalPriceSeries(
    std::string_view expectedSymbol,
    const std::vector<HistoricalBar>& bars,
    HistoricalPriceField priceField)
{
    const std::string symbol = market::normalizeSymbol(expectedSymbol);
    if (symbol.empty()) {
        return failure(HistoricalPriceSeriesError::InvalidExpectedSymbol);
    }
    if (bars.empty()) {
        return failure(HistoricalPriceSeriesError::EmptyBars);
    }

    HistoricalPriceSeriesResult result;
    result.symbol = symbol;
    result.priceField = priceField;
    result.prices.reserve(bars.size());

    std::int64_t previousTimestampMs = 0;
    for (std::size_t index = 0; index < bars.size(); ++index) {
        const HistoricalBar& bar = bars[index];
        const std::string barSymbol = market::normalizeSymbol(bar.symbol);
        if (barSymbol.empty()) {
            return failure(HistoricalPriceSeriesError::InvalidBarSymbol,
                           "Historical bar symbol is empty at index "
                               + std::to_string(index));
        }
        if (barSymbol != symbol) {
            return failure(HistoricalPriceSeriesError::SymbolMismatch,
                           "Historical bar symbol does not match " + symbol
                               + " at index " + std::to_string(index));
        }

        const auto timestampMs = tradingDateToTimestampMs(bar.tradingDate);
        if (!timestampMs) {
            return failure(HistoricalPriceSeriesError::InvalidTradingDate,
                           "Invalid ISO trading date at index " + std::to_string(index));
        }
        if (index > 0 && *timestampMs <= previousTimestampMs) {
            return failure(HistoricalPriceSeriesError::TradingDatesNotStrictlyIncreasing);
        }

        double price = bar.close;
        if (priceField == HistoricalPriceField::Close) {
            if (!isValidPrice(price)) {
                return failure(HistoricalPriceSeriesError::InvalidClose,
                               "Invalid close price at index " + std::to_string(index));
            }
        } else {
            if (!bar.adjustedClose) {
                return failure(HistoricalPriceSeriesError::MissingAdjustedClose,
                               "Adjusted close is missing at index "
                                   + std::to_string(index));
            }
            price = *bar.adjustedClose;
            if (!isValidPrice(price)) {
                return failure(HistoricalPriceSeriesError::InvalidAdjustedClose,
                               "Invalid adjusted close at index "
                                   + std::to_string(index));
            }
        }

        result.prices.push_back({*timestampMs, price});
        previousTimestampMs = *timestampMs;
    }
    return result;
}

std::string historicalPriceSeriesErrorMessage(HistoricalPriceSeriesError error)
{
    switch (error) {
    case HistoricalPriceSeriesError::None: return {};
    case HistoricalPriceSeriesError::InvalidExpectedSymbol: return "Expected symbol is empty";
    case HistoricalPriceSeriesError::EmptyBars: return "Historical bar series is empty";
    case HistoricalPriceSeriesError::InvalidBarSymbol: return "Historical bar symbol is empty";
    case HistoricalPriceSeriesError::SymbolMismatch:
        return "Historical bar symbol does not match the expected symbol";
    case HistoricalPriceSeriesError::InvalidTradingDate:
        return "Trading date must be a valid ISO date after 1970-01-01";
    case HistoricalPriceSeriesError::TradingDatesNotStrictlyIncreasing:
        return "Historical trading dates must be strictly increasing";
    case HistoricalPriceSeriesError::InvalidClose:
        return "Close price must be finite and greater than zero";
    case HistoricalPriceSeriesError::MissingAdjustedClose:
        return "Adjusted close is required by the selected price convention";
    case HistoricalPriceSeriesError::InvalidAdjustedClose:
        return "Adjusted close must be finite and greater than zero";
    }
    return "Unknown historical price series error";
}

} // namespace fininsight::simulation
