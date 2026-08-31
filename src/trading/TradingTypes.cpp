#include "trading/TradingTypes.h"

namespace fininsight::trading {

OrderStatus statusAfterFill(const Order& order, std::int64_t fillQuantity)
{
    if (fillQuantity <= 0 || order.request.quantity <= 0) return order.status;
    const auto total = order.filledQuantity + fillQuantity;
    return total >= order.request.quantity ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
}

bool isTerminal(OrderStatus status)
{
    return status == OrderStatus::Filled || status == OrderStatus::Cancelled
        || status == OrderStatus::Rejected || status == OrderStatus::Expired
        || status == OrderStatus::RiskRejected;
}

const char* orderStatusName(OrderStatus status)
{
    switch (status) {
    case OrderStatus::Created: return "Created";
    case OrderStatus::RiskRejected: return "RiskRejected";
    case OrderStatus::Submitting: return "Submitting";
    case OrderStatus::Accepted: return "Accepted";
    case OrderStatus::PartiallyFilled: return "PartiallyFilled";
    case OrderStatus::Filled: return "Filled";
    case OrderStatus::CancelPending: return "CancelPending";
    case OrderStatus::Cancelled: return "Cancelled";
    case OrderStatus::Rejected: return "Rejected";
    case OrderStatus::Expired: return "Expired";
    case OrderStatus::Unknown: return "Unknown";
    }
    return "Unknown";
}

} // namespace fininsight::trading
