#pragma once
#include "trading/ExecutionGateway.h"
#include <unordered_map>
namespace fininsight::trading {
class PaperExecutionGateway final : public ExecutionGateway {
public:
    explicit PaperExecutionGateway(double initialCash, double feeRate = 0.001);
    std::vector<Order> onQuote(const std::string& symbol, double price, std::int64_t timestampMs);
    Order submit(const OrderRequest& request) override;
    bool cancel(const std::string& clientOrderId) override;
    std::optional<Order> find(const std::string& clientOrderId) const override;
    std::vector<Order> orders() const override;
    AccountSnapshot account() const override;
    std::int64_t position(const std::string& symbol) const;
private:
    void tryFill(Order& order);
    double cash_ = 0.0;
    double initialCash_ = 0.0;
    double feeRate_ = 0.001;
    std::unordered_map<std::string, double> prices_;
    std::unordered_map<std::string, std::int64_t> positions_;
    std::unordered_map<std::string, Order> orders_;
};
} // namespace fininsight::trading
