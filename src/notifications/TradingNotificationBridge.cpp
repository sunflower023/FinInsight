#include "notifications/TradingNotificationBridge.h"
#include "notifications/NotificationService.h"

namespace fininsight::notifications {

void TradingNotificationBridge::onOrderChanged(const trading::Order& order)
{
    Severity severity = Severity::Info;
    if (order.status == trading::OrderStatus::RiskRejected || order.status == trading::OrderStatus::Rejected)
        severity = Severity::Warning;
    else if (order.status == trading::OrderStatus::Unknown)
        severity = Severity::Critical;

    const QString side = order.request.side == trading::OrderSide::Buy ? QStringLiteral("BUY") : QStringLiteral("SELL");
    const QString id = QString::fromStdString(order.request.clientOrderId);
    const QString message = order.rejectionReason.empty()
        ? QStringLiteral("%1 %2 x %3").arg(side, QString::fromStdString(order.request.symbol)).arg(order.request.quantity)
        : QString::fromStdString(order.rejectionReason);
    service_.publish({0, QStringLiteral("trading"),
                      QStringLiteral("Order %1").arg(QString::fromLatin1(trading::orderStatusName(order.status))),
                      message, QStringLiteral("order:%1:%2").arg(id).arg(int(order.status)), severity});
}

} // namespace fininsight::notifications
