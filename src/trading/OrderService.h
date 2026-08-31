#pragma once
#include "trading/ExecutionGateway.h"
#include "trading/RiskEngine.h"
#include <unordered_set>
namespace fininsight::trading {
class OrderService final {
public:
    OrderService(ExecutionGateway& gateway, RiskEngine riskEngine);
    Order submit(const OrderRequest& request, double referencePrice, const RiskContext& context);
    bool cancel(const std::string& clientOrderId);
private:
    ExecutionGateway& gateway_; RiskEngine riskEngine_; std::unordered_set<std::string> clientIds_;
};
} // namespace fininsight::trading
