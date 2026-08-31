#pragma once

#include <cstdint>
#include <string>

namespace fininsight::trading {

enum class OrderSide { Buy, Sell };
enum class OrderType { Market, Limit };
enum class OrderStatus {
    Created, RiskRejected, Submitting, Accepted, PartiallyFilled, Filled,
    CancelPending, Cancelled, Rejected, Expired, Unknown
};

struct OrderRequest {
    std::string clientOrderId;
    std::string symbol;
    OrderSide side = OrderSide::Buy;
    OrderType type = OrderType::Market;
    std::int64_t quantity = 0;
    double limitPrice = 0.0;
    std::int64_t timestampMs = 0;
};

struct Order {
    OrderRequest request;
    OrderStatus status = OrderStatus::Created;
    std::string brokerOrderId;
    std::int64_t filledQuantity = 0;
    double averageFillPrice = 0.0;
    std::string rejectionReason;
};

struct Fill {
    std::string clientOrderId;
    std::string brokerOrderId;
    std::int64_t quantity = 0;
    double price = 0.0;
    double fee = 0.0;
    std::int64_t timestampMs = 0;
};

struct AccountSnapshot {
    double cash = 0.0;
    double buyingPower = 0.0;
    double equity = 0.0;
};

OrderStatus statusAfterFill(const Order& order, std::int64_t fillQuantity);
bool isTerminal(OrderStatus status);
const char* orderStatusName(OrderStatus status);

} // namespace fininsight::trading
