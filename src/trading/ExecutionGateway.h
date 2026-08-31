#pragma once
#include "trading/TradingTypes.h"
#include <optional>
#include <string>
#include <vector>
namespace fininsight::trading {
class ExecutionGateway {
public:
    virtual ~ExecutionGateway() = default;
    virtual Order submit(const OrderRequest& request) = 0;
    virtual bool cancel(const std::string& clientOrderId) = 0;
    virtual std::optional<Order> find(const std::string& clientOrderId) const = 0;
    virtual std::vector<Order> orders() const = 0;
    virtual AccountSnapshot account() const = 0;
};
} // namespace fininsight::trading
