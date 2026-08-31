#include "trading/RiskEngine.h"

#include <cmath>

namespace fininsight::trading {

RiskEngine::RiskEngine(RiskLimits limits) : limits_(limits) {}

RiskCheckResult RiskEngine::check(const OrderRequest& request, double referencePrice,
                                  const RiskContext& context) const
{
    auto reject = [](const char* reason) { return RiskCheckResult{false, reason}; };
    if (context.killSwitch) return reject("Trading is stopped by kill switch");
    if (!context.marketOpen) return reject("Market is closed");
    if (request.clientOrderId.empty() || request.symbol.empty()) return reject("Missing order identity");
    if (request.quantity <= 0 || request.quantity > limits_.maxOrderQuantity) return reject("Quantity exceeds limit");
    if (!std::isfinite(referencePrice) || referencePrice <= 0.0) return reject("No valid reference price");
    if (request.type == OrderType::Limit && (!std::isfinite(request.limitPrice) || request.limitPrice <= 0.0))
        return reject("Limit price is invalid");
    const double notional = static_cast<double>(request.quantity)
        * (request.type == OrderType::Limit ? request.limitPrice : referencePrice);
    if (!std::isfinite(notional) || notional > limits_.maxOrderNotional) return reject("Order notional exceeds limit");
    if (context.currentSymbolExposure + notional > limits_.maxSymbolExposure) return reject("Symbol exposure exceeds limit");
    if (context.dailyOrderCount >= limits_.maxDailyOrders) return reject("Daily order limit reached");
    if (context.dailyRealizedLoss >= limits_.maxDailyLoss) return reject("Daily loss limit reached");
    if (context.quoteAgeMs < 0 || context.quoteAgeMs > limits_.quoteFreshnessMs) return reject("Quote is stale");
    return {true, {}};
}

} // namespace fininsight::trading
