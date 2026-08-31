#include "trading/OrderService.h"
namespace fininsight::trading {
OrderService::OrderService(ExecutionGateway& gateway, RiskEngine riskEngine) : gateway_(gateway), riskEngine_(std::move(riskEngine)) {}
Order OrderService::submit(const OrderRequest& request, double referencePrice, const RiskContext& context)
{
    Order order; order.request = request;
    if (clientIds_.contains(request.clientOrderId)) { order.status = OrderStatus::Rejected; order.rejectionReason = "Duplicate client order ID"; return order; }
    const auto risk = riskEngine_.check(request, referencePrice, context);
    if (!risk.approved) { order.status = OrderStatus::RiskRejected; order.rejectionReason = risk.reason; return order; }
    clientIds_.insert(request.clientOrderId); return gateway_.submit(request);
}
bool OrderService::cancel(const std::string& id) { return gateway_.cancel(id); }
} // namespace fininsight::trading
