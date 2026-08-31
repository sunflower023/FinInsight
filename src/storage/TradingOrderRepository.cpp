#include "storage/TradingOrderRepository.h"
#include "storage/Database.h"
#include <QDateTime>
#include <QSqlQuery>
#include <algorithm>
namespace fininsight::storage {
TradingOrderRepository::TradingOrderRepository(Database& database) : database_(database) {}
bool TradingOrderRepository::save(const trading::Order& order, const char* environment)
{
    QSqlQuery q(database_.mainConnection());
    q.prepare(R"(INSERT INTO trading_orders
        (client_order_id,broker_order_id,environment,symbol,side,order_type,quantity,limit_price,status,filled_quantity,average_fill_price,rejection_reason,created_at_ms,updated_at_ms)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)
        ON CONFLICT(client_order_id) DO UPDATE SET broker_order_id=excluded.broker_order_id,status=excluded.status,
        filled_quantity=excluded.filled_quantity,average_fill_price=excluded.average_fill_price,rejection_reason=excluded.rejection_reason,updated_at_ms=excluded.updated_at_ms)");
    const auto now = QDateTime::currentMSecsSinceEpoch();
    q.addBindValue(QString::fromStdString(order.request.clientOrderId)); q.addBindValue(QString::fromStdString(order.brokerOrderId));
    q.addBindValue(QString::fromLatin1(environment)); q.addBindValue(QString::fromStdString(order.request.symbol));
    q.addBindValue(order.request.side == trading::OrderSide::Buy ? 0 : 1); q.addBindValue(order.request.type == trading::OrderType::Market ? 0 : 1);
    q.addBindValue(qlonglong(order.request.quantity)); q.addBindValue(order.request.limitPrice); q.addBindValue(static_cast<int>(order.status));
    q.addBindValue(qlonglong(order.filledQuantity)); q.addBindValue(order.averageFillPrice); q.addBindValue(QString::fromStdString(order.rejectionReason));
    q.addBindValue(qlonglong(order.request.timestampMs)); q.addBindValue(qlonglong(now)); return q.exec();
}
std::vector<trading::Order> TradingOrderRepository::recent(int limit) const
{
    std::vector<trading::Order> result; QSqlQuery q(database_.mainConnection());
    q.prepare("SELECT client_order_id,broker_order_id,symbol,side,order_type,quantity,limit_price,status,filled_quantity,average_fill_price,rejection_reason,created_at_ms FROM trading_orders ORDER BY updated_at_ms DESC LIMIT ?");
    q.addBindValue(std::clamp(limit, 1, 1000)); if (!q.exec()) return result;
    while (q.next()) { trading::Order o; o.request.clientOrderId=q.value(0).toString().toStdString(); o.brokerOrderId=q.value(1).toString().toStdString();
        o.request.symbol=q.value(2).toString().toStdString(); o.request.side=q.value(3).toInt()==0?trading::OrderSide::Buy:trading::OrderSide::Sell;
        o.request.type=q.value(4).toInt()==0?trading::OrderType::Market:trading::OrderType::Limit; o.request.quantity=q.value(5).toLongLong(); o.request.limitPrice=q.value(6).toDouble();
        o.status=static_cast<trading::OrderStatus>(q.value(7).toInt()); o.filledQuantity=q.value(8).toLongLong(); o.averageFillPrice=q.value(9).toDouble();
        o.rejectionReason=q.value(10).toString().toStdString(); o.request.timestampMs=q.value(11).toLongLong(); result.push_back(std::move(o)); } return result;
}
}
