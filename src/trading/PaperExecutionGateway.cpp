#include "trading/PaperExecutionGateway.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace fininsight::trading {
PaperExecutionGateway::PaperExecutionGateway(double initialCash, double feeRate)
    : cash_(initialCash), initialCash_(initialCash), feeRate_(feeRate)
{
    if (!std::isfinite(initialCash) || initialCash <= 0.0) throw std::invalid_argument("invalid initial cash");
}
std::vector<Order> PaperExecutionGateway::onQuote(const std::string& symbol, double price, std::int64_t)
{
    std::vector<Order> changedOrders;
    if (symbol.empty() || !std::isfinite(price) || price <= 0.0) return changedOrders;
    prices_[symbol] = price;
    for (auto& [id, order] : orders_) {
        if (order.request.symbol != symbol || (order.status != OrderStatus::Accepted && order.status != OrderStatus::PartiallyFilled)) continue;
        const auto previousStatus = order.status;
        const auto previousFilledQuantity = order.filledQuantity;
        tryFill(order);
        if (order.status != previousStatus || order.filledQuantity != previousFilledQuantity) changedOrders.push_back(order);
    }
    return changedOrders;
}
Order PaperExecutionGateway::submit(const OrderRequest& request)
{
    if (orders_.contains(request.clientOrderId)) return orders_.at(request.clientOrderId);
    Order order; order.request = request; order.status = OrderStatus::Accepted;
    orders_.emplace(request.clientOrderId, order);
    tryFill(orders_.at(request.clientOrderId));
    return orders_.at(request.clientOrderId);
}
bool PaperExecutionGateway::cancel(const std::string& id)
{
    auto it = orders_.find(id);
    if (it == orders_.end() || isTerminal(it->second.status)) return false;
    it->second.status = OrderStatus::Cancelled; return true;
}
std::optional<Order> PaperExecutionGateway::find(const std::string& id) const
{ auto it = orders_.find(id); return it == orders_.end() ? std::nullopt : std::optional<Order>(it->second); }
std::vector<Order> PaperExecutionGateway::orders() const
{
    std::vector<Order> result;
    for (const auto& [id, order] : orders_) result.push_back(order);
    std::sort(result.begin(), result.end(), [](const Order& lhs, const Order& rhs) {
        if (lhs.request.timestampMs != rhs.request.timestampMs) return lhs.request.timestampMs < rhs.request.timestampMs;
        return lhs.request.clientOrderId < rhs.request.clientOrderId;
    });
    return result;
}
AccountSnapshot PaperExecutionGateway::account() const
{
    double holdings = 0.0;
    for (const auto& [symbol, quantity] : positions_) { auto it = prices_.find(symbol); if (it != prices_.end()) holdings += it->second * quantity; }
    return {cash_, cash_, cash_ + holdings};
}
std::int64_t PaperExecutionGateway::position(const std::string& symbol) const
{ auto it = positions_.find(symbol); return it == positions_.end() ? 0 : it->second; }
void PaperExecutionGateway::tryFill(Order& order)
{
    auto priceIt = prices_.find(order.request.symbol); if (priceIt == prices_.end()) return;
    const double price = priceIt->second;
    if (order.request.type == OrderType::Limit) {
        if (order.request.side == OrderSide::Buy && price > order.request.limitPrice) return;
        if (order.request.side == OrderSide::Sell && price < order.request.limitPrice) return;
    }
    const double value = price * order.request.quantity; const double fee = value * feeRate_;
    if (order.request.side == OrderSide::Buy) {
        if (cash_ < value + fee) { order.status = OrderStatus::Rejected; order.rejectionReason = "Insufficient paper cash"; return; }
        cash_ -= value + fee; positions_[order.request.symbol] += order.request.quantity;
    } else {
        if (position(order.request.symbol) < order.request.quantity) { order.status = OrderStatus::Rejected; order.rejectionReason = "Insufficient paper position"; return; }
        cash_ += value - fee; positions_[order.request.symbol] -= order.request.quantity;
    }
    order.filledQuantity = order.request.quantity; order.averageFillPrice = price; order.status = OrderStatus::Filled;
}
} // namespace fininsight::trading
