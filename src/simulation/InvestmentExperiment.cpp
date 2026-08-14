#include "simulation/InvestmentExperiment.h"

#include "market/QuoteRules.h"
#include "simulation/Ledger.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <unordered_map>
#include <utility>

namespace fininsight::simulation {
namespace {

InvestmentExperimentResult failure(InvestmentExperimentError error,
                                   std::string detail = {})
{
    InvestmentExperimentResult result;
    result.error = error;
    result.message = detail.empty() ? investmentExperimentErrorMessage(error)
                                    : std::move(detail);
    return result;
}

} // namespace

InvestmentExperimentResult runBuyAndHoldExperiment(
    const InvestmentExperimentRequest& request,
    const std::vector<PricePoint>& prices)
{
    const std::string symbol = market::normalizeSymbol(request.symbol);
    if (symbol.empty()) {
        return failure(InvestmentExperimentError::InvalidSymbol);
    }
    if (!std::isfinite(request.initialCash) || request.initialCash <= 0.0) {
        return failure(InvestmentExperimentError::InvalidInitialCash);
    }
    if (request.startTimestampMs <= 0 || request.endTimestampMs <= 0
        || request.startTimestampMs > request.endTimestampMs) {
        return failure(InvestmentExperimentError::InvalidTimeRange);
    }
    if (!std::isfinite(request.buyFee) || request.buyFee < 0.0) {
        return failure(InvestmentExperimentError::InvalidFee);
    }
    if (prices.empty()) {
        return failure(InvestmentExperimentError::EmptyPriceSeries);
    }

    for (std::size_t index = 0; index < prices.size(); ++index) {
        const PricePoint& point = prices[index];
        if (point.timestampMs <= 0 || !std::isfinite(point.price) || point.price <= 0.0) {
            return failure(InvestmentExperimentError::InvalidPricePoint);
        }
        if (index > 0 && point.timestampMs <= prices[index - 1].timestampMs) {
            return failure(InvestmentExperimentError::PriceSeriesNotStrictlyIncreasing);
        }
    }

    const auto first = std::lower_bound(
        prices.begin(), prices.end(), request.startTimestampMs,
        [](const PricePoint& point, std::int64_t timestampMs) {
            return point.timestampMs < timestampMs;
        });
    const auto afterLast = std::upper_bound(
        prices.begin(), prices.end(), request.endTimestampMs,
        [](std::int64_t timestampMs, const PricePoint& point) {
            return timestampMs < point.timestampMs;
        });
    if (first == prices.end() || first == afterLast) {
        return failure(InvestmentExperimentError::NoPriceInRange);
    }
    const auto last = std::prev(afterLast);

    const double spendableCash = request.initialCash - request.buyFee;
    if (spendableCash < first->price) {
        return failure(InvestmentExperimentError::InsufficientCash);
    }
    const long double affordableQuantity = std::floor(
        static_cast<long double>(spendableCash) / static_cast<long double>(first->price));
    if (!std::isfinite(affordableQuantity)
        || affordableQuantity > static_cast<long double>(
            std::numeric_limits<std::int64_t>::max())) {
        return failure(InvestmentExperimentError::QuantityOverflow);
    }
    const auto quantity = static_cast<std::int64_t>(affordableQuantity);
    if (quantity <= 0) {
        return failure(InvestmentExperimentError::InsufficientCash);
    }

    Ledger ledger(request.initialCash);
    const TradeResult trade = ledger.execute({TradeSide::Buy, symbol, quantity,
                                               first->price, request.buyFee,
                                               first->timestampMs});
    if (!trade.ok()) {
        return failure(InvestmentExperimentError::TradeRejected, trade.message);
    }

    double runningPeak = 0.0;
    double maxDrawdown = 0.0;
    for (auto point = first; point != afterLast; ++point) {
        const ValuationResult valuation = ledger.value({{symbol, point->price}});
        if (!valuation.valid || !std::isfinite(valuation.snapshot.totalEquity)) {
            return failure(InvestmentExperimentError::ValuationFailed,
                           valuation.error.empty() ? investmentExperimentErrorMessage(
                               InvestmentExperimentError::ValuationFailed) : valuation.error);
        }
        const double equity = valuation.snapshot.totalEquity;
        runningPeak = std::max(runningPeak, equity);
        if (runningPeak > 0.0) {
            maxDrawdown = std::max(maxDrawdown, (runningPeak - equity) / runningPeak);
        }
    }

    const ValuationResult endingValuation = ledger.value({{symbol, last->price}});
    if (!endingValuation.valid || !std::isfinite(endingValuation.snapshot.totalEquity)) {
        return failure(InvestmentExperimentError::ValuationFailed,
                       endingValuation.error.empty() ? investmentExperimentErrorMessage(
                           InvestmentExperimentError::ValuationFailed) : endingValuation.error);
    }

    InvestmentExperimentResult result;
    result.symbol = symbol;
    result.executedTimestampMs = first->timestampMs;
    result.executedPrice = first->price;
    result.endingTimestampMs = last->timestampMs;
    result.endingPrice = last->price;
    result.quantity = quantity;
    result.endingCash = endingValuation.snapshot.cash;
    result.endingMarketValue = endingValuation.snapshot.holdingsValue;
    result.endingEquity = endingValuation.snapshot.totalEquity;
    result.totalPnl = endingValuation.snapshot.totalPnl;
    result.returnRate = endingValuation.snapshot.returnRate;
    result.maxDrawdown = maxDrawdown;
    return result;
}

std::string investmentExperimentErrorMessage(InvestmentExperimentError error)
{
    switch (error) {
    case InvestmentExperimentError::None: return {};
    case InvestmentExperimentError::InvalidSymbol: return "Symbol is empty";
    case InvestmentExperimentError::InvalidInitialCash:
        return "Initial cash must be finite and greater than zero";
    case InvestmentExperimentError::InvalidTimeRange: return "Invalid experiment time range";
    case InvestmentExperimentError::InvalidFee: return "Buy fee must be finite and non-negative";
    case InvestmentExperimentError::EmptyPriceSeries: return "Price series is empty";
    case InvestmentExperimentError::InvalidPricePoint:
        return "Every price point must have a positive timestamp and finite positive price";
    case InvestmentExperimentError::PriceSeriesNotStrictlyIncreasing:
        return "Price timestamps must be strictly increasing";
    case InvestmentExperimentError::NoPriceInRange:
        return "No price is available in the requested time range";
    case InvestmentExperimentError::InsufficientCash:
        return "Initial cash cannot buy one share after fees";
    case InvestmentExperimentError::QuantityOverflow: return "Share quantity exceeds supported range";
    case InvestmentExperimentError::TradeRejected: return "Ledger rejected the buy trade";
    case InvestmentExperimentError::ValuationFailed: return "Portfolio valuation failed";
    }
    return "Unknown investment experiment error";
}

} // namespace fininsight::simulation
