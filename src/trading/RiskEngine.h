#pragma once

#include "trading/TradingTypes.h"

#include <string>
#include <unordered_map>

namespace fininsight::trading {

struct RiskLimits {
    double maxOrderNotional = 10000.0;
    std::int64_t maxOrderQuantity = 1000;
    double maxSymbolExposure = 25000.0;
    std::int64_t maxDailyOrders = 100;
    double maxDailyLoss = 1000.0;
    std::int64_t quoteFreshnessMs = 30000;
};

struct RiskContext {
    double currentSymbolExposure = 0.0;
    std::int64_t dailyOrderCount = 0;
    double dailyRealizedLoss = 0.0;
    std::int64_t quoteAgeMs = 0;
    bool marketOpen = true;
    bool killSwitch = false;
};

struct RiskCheckResult {
    bool approved = false;
    std::string reason;
};

class RiskEngine final {
public:
    explicit RiskEngine(RiskLimits limits = {});
    RiskCheckResult check(const OrderRequest& request, double referencePrice,
                          const RiskContext& context) const;
    const RiskLimits& limits() const { return limits_; }

private:
    RiskLimits limits_;
};

} // namespace fininsight::trading
