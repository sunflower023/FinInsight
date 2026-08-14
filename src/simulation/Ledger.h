#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fininsight::simulation {

enum class TradeSide {
    Buy,
    Sell,
};

enum class TradeError {
    None,
    InvalidSymbol,
    InvalidQuantity,
    InvalidPrice,
    InvalidFee,
    InsufficientCash,
    InsufficientPosition,
    FeeExceedsProceeds,
};

struct TradeRequest {
    TradeSide side = TradeSide::Buy;
    std::string symbol;
    std::int64_t quantity = 0;
    double price = 0.0;
    double fee = 0.0;
    std::int64_t timestampMs = 0;
};

struct Trade {
    std::uint64_t id = 0;
    TradeSide side = TradeSide::Buy;
    std::string symbol;
    std::int64_t quantity = 0;
    double price = 0.0;
    double fee = 0.0;
    std::int64_t timestampMs = 0;
    double realizedPnl = 0.0;
};

struct Position {
    std::string symbol;
    std::int64_t quantity = 0;
    double totalCost = 0.0;

    double averageCost() const;
};

struct TradeResult {
    TradeError error = TradeError::None;
    std::string message;
    Trade trade;

    bool ok() const { return error == TradeError::None; }
};

struct PositionValuation {
    Position position;
    double marketPrice = 0.0;
    double marketValue = 0.0;
    double unrealizedPnl = 0.0;
};

struct PortfolioSnapshot {
    double initialCash = 0.0;
    double cash = 0.0;
    double holdingsValue = 0.0;
    double totalEquity = 0.0;
    double realizedPnl = 0.0;
    double unrealizedPnl = 0.0;
    double totalPnl = 0.0;
    double returnRate = 0.0;
    std::vector<PositionValuation> positions;
};

struct ValuationResult {
    bool valid = false;
    std::string error;
    PortfolioSnapshot snapshot;
};

class Ledger {
public:
    explicit Ledger(double initialCash);

    TradeResult execute(const TradeRequest& request);
    ValuationResult value(const std::unordered_map<std::string, double>& prices) const;

    double initialCash() const { return initialCash_; }
    double cash() const { return cash_; }
    double realizedPnl() const { return realizedPnl_; }
    const std::unordered_map<std::string, Position>& positions() const { return positions_; }
    const std::vector<Trade>& trades() const { return trades_; }

private:
    TradeResult buy(const TradeRequest& request);
    TradeResult sell(const TradeRequest& request);
    TradeResult validate(const TradeRequest& request) const;

    double initialCash_ = 0.0;
    double cash_ = 0.0;
    double realizedPnl_ = 0.0;
    std::uint64_t nextTradeId_ = 1;
    std::unordered_map<std::string, Position> positions_;
    std::vector<Trade> trades_;
};

std::string tradeErrorMessage(TradeError error);

} // namespace fininsight::simulation
