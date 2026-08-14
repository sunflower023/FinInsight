#include "simulation/Ledger.h"

#include "market/QuoteRules.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace fininsight::simulation {
namespace {

constexpr double kEpsilon = 1e-9;

bool isValidMoney(double value)
{
    return std::isfinite(value) && value >= 0.0;
}

} // namespace

double Position::averageCost() const
{
    return quantity > 0 ? totalCost / static_cast<double>(quantity) : 0.0;
}

Ledger::Ledger(double initialCash)
    : initialCash_(initialCash)
    , cash_(initialCash)
{
    if (!isValidMoney(initialCash)) {
        throw std::invalid_argument("Initial cash must be finite and non-negative");
    }
}

TradeResult Ledger::execute(const TradeRequest& request)
{
    const TradeResult validation = validate(request);
    if (!validation.ok()) return validation;
    return request.side == TradeSide::Buy ? buy(request) : sell(request);
}

TradeResult Ledger::validate(const TradeRequest& request) const
{
    if (market::normalizeSymbol(request.symbol).empty()) {
        return {TradeError::InvalidSymbol, tradeErrorMessage(TradeError::InvalidSymbol), {}};
    }
    if (request.quantity <= 0) {
        return {TradeError::InvalidQuantity, tradeErrorMessage(TradeError::InvalidQuantity), {}};
    }
    if (!std::isfinite(request.price) || request.price <= 0.0) {
        return {TradeError::InvalidPrice, tradeErrorMessage(TradeError::InvalidPrice), {}};
    }
    if (!isValidMoney(request.fee)) {
        return {TradeError::InvalidFee, tradeErrorMessage(TradeError::InvalidFee), {}};
    }
    return {};
}

TradeResult Ledger::buy(const TradeRequest& request)
{
    const std::string symbol = market::normalizeSymbol(request.symbol);
    const double gross = request.price * static_cast<double>(request.quantity);
    const double cashRequired = gross + request.fee;
    if (!std::isfinite(cashRequired) || cashRequired > cash_ + kEpsilon) {
        return {TradeError::InsufficientCash, tradeErrorMessage(TradeError::InsufficientCash), {}};
    }

    auto& position = positions_[symbol];
    position.symbol = symbol;
    position.quantity += request.quantity;
    position.totalCost += cashRequired;
    cash_ -= cashRequired;
    if (std::abs(cash_) < kEpsilon) cash_ = 0.0;

    Trade trade{nextTradeId_++, TradeSide::Buy, symbol, request.quantity,
                request.price, request.fee, request.timestampMs, 0.0};
    trades_.push_back(trade);
    return {TradeError::None, {}, trade};
}

TradeResult Ledger::sell(const TradeRequest& request)
{
    const std::string symbol = market::normalizeSymbol(request.symbol);
    auto positionIt = positions_.find(symbol);
    if (positionIt == positions_.end() || positionIt->second.quantity < request.quantity) {
        return {TradeError::InsufficientPosition,
                tradeErrorMessage(TradeError::InsufficientPosition), {}};
    }

    const double gross = request.price * static_cast<double>(request.quantity);
    if (!std::isfinite(gross) || request.fee > gross + kEpsilon) {
        return {TradeError::FeeExceedsProceeds,
                tradeErrorMessage(TradeError::FeeExceedsProceeds), {}};
    }

    Position& position = positionIt->second;
    const double removedCost = position.averageCost() * static_cast<double>(request.quantity);
    const double proceeds = gross - request.fee;
    const double tradePnl = proceeds - removedCost;

    position.quantity -= request.quantity;
    position.totalCost -= removedCost;
    cash_ += proceeds;
    realizedPnl_ += tradePnl;
    if (position.quantity == 0) positions_.erase(positionIt);

    Trade trade{nextTradeId_++, TradeSide::Sell, symbol, request.quantity,
                request.price, request.fee, request.timestampMs, tradePnl};
    trades_.push_back(trade);
    return {TradeError::None, {}, trade};
}

ValuationResult Ledger::value(const std::unordered_map<std::string, double>& prices) const
{
    PortfolioSnapshot snapshot;
    snapshot.initialCash = initialCash_;
    snapshot.cash = cash_;
    snapshot.realizedPnl = realizedPnl_;

    for (const auto& [symbol, position] : positions_) {
        auto priceIt = prices.find(symbol);
        if (priceIt == prices.end()) {
            return {false, "Missing market price for " + symbol, {}};
        }
        const double marketPrice = priceIt->second;
        if (!std::isfinite(marketPrice) || marketPrice <= 0.0) {
            return {false, "Invalid market price for " + symbol, {}};
        }

        PositionValuation valuation;
        valuation.position = position;
        valuation.marketPrice = marketPrice;
        valuation.marketValue = marketPrice * static_cast<double>(position.quantity);
        valuation.unrealizedPnl = valuation.marketValue - position.totalCost;
        snapshot.holdingsValue += valuation.marketValue;
        snapshot.unrealizedPnl += valuation.unrealizedPnl;
        snapshot.positions.push_back(std::move(valuation));
    }

    snapshot.totalEquity = snapshot.cash + snapshot.holdingsValue;
    snapshot.totalPnl = snapshot.realizedPnl + snapshot.unrealizedPnl;
    snapshot.returnRate = initialCash_ > 0.0
        ? snapshot.totalPnl / initialCash_ : 0.0;
    return {true, {}, std::move(snapshot)};
}

std::string tradeErrorMessage(TradeError error)
{
    switch (error) {
    case TradeError::None: return {};
    case TradeError::InvalidSymbol: return "Symbol is empty";
    case TradeError::InvalidQuantity: return "Quantity must be greater than zero";
    case TradeError::InvalidPrice: return "Price must be finite and greater than zero";
    case TradeError::InvalidFee: return "Fee must be finite and non-negative";
    case TradeError::InsufficientCash: return "Insufficient cash";
    case TradeError::InsufficientPosition: return "Insufficient position";
    case TradeError::FeeExceedsProceeds: return "Fee exceeds sale proceeds";
    }
    return "Unknown trade error";
}

} // namespace fininsight::simulation
