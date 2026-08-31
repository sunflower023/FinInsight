#include "trading/AlpacaBrokerClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QUrl>

namespace fininsight::trading {
namespace {
OrderStatus parseStatus(const QString& status)
{
    if (status == "new" || status == "accepted" || status == "pending_new") return OrderStatus::Accepted;
    if (status == "partially_filled") return OrderStatus::PartiallyFilled;
    if (status == "filled") return OrderStatus::Filled;
    if (status == "pending_cancel") return OrderStatus::CancelPending;
    if (status == "canceled") return OrderStatus::Cancelled;
    if (status == "rejected") return OrderStatus::Rejected;
    if (status == "expired") return OrderStatus::Expired;
    return OrderStatus::Unknown;
}

QString responseError(const network::HttpResponse& response)
{
    const auto object = QJsonDocument::fromJson(response.body).object();
    const QString message = object.value(QStringLiteral("message")).toString();
    return message.isEmpty() ? (response.error.isEmpty() ? QStringLiteral("Broker request failed") : response.error) : message;
}
}

AlpacaBrokerClient::AlpacaBrokerClient(Config config, QObject* parent)
    : QObject(parent), config_(std::move(config))
{
    if (config_.baseUrl.trimmed().isEmpty()) config_.baseUrl = defaultBaseUrl(config_.environment);
    while (config_.baseUrl.endsWith('/')) config_.baseUrl.chop(1);
}

QString AlpacaBrokerClient::defaultBaseUrl(Environment environment)
{
    return environment == Environment::Paper
        ? QStringLiteral("https://paper-api.alpaca.markets")
        : QStringLiteral("https://api.alpaca.markets");
}

bool AlpacaBrokerClient::isConfigured() const
{
    return !config_.baseUrl.isEmpty() && !config_.apiKey.isEmpty() && !config_.secretKey.isEmpty() && config_.timeoutMs > 0;
}

bool AlpacaBrokerClient::canMutateOrders() const
{
    return isConfigured() && (config_.environment == Environment::Paper || config_.liveTradingUnlocked);
}

QString AlpacaBrokerClient::configurationError(bool mutation) const
{
    if (!isConfigured()) return QStringLiteral("Alpaca credentials are not configured");
    if (mutation && config_.environment == Environment::Live && !config_.liveTradingUnlocked)
        return QStringLiteral("Live trading is locked");
    return {};
}

QMap<QByteArray, QByteArray> AlpacaBrokerClient::authHeaders() const
{
    QMap<QByteArray, QByteArray> headers;
    headers.insert(QByteArrayLiteral("APCA-API-KEY-ID"), config_.apiKey.toUtf8());
    headers.insert(QByteArrayLiteral("APCA-API-SECRET-KEY"), config_.secretKey.toUtf8());
    return headers;
}

QString AlpacaBrokerClient::endpoint(const QString& path) const { return config_.baseUrl + path; }

network::HttpClient::RequestId AlpacaBrokerClient::fetchAccount(QObject* context, AccountHandler handler)
{
    const QString error = configurationError(false);
    if (!error.isEmpty()) { QMetaObject::invokeMethod(context, [handler=std::move(handler),error]{ handler({false,{},error}); }, Qt::QueuedConnection); return network::HttpClient::InvalidRequestId; }
    return network::HttpClient::instance().getAsync(endpoint(QStringLiteral("/v2/account")), context,
        [handler=std::move(handler)](const network::HttpResponse& response) {
            if (!response.isSuccess()) { handler({false,{},responseError(response)}); return; }
            QJsonParseError parseError; const auto object = QJsonDocument::fromJson(response.body, &parseError).object();
            if (parseError.error != QJsonParseError::NoError || object.isEmpty()) { handler({false,{},QStringLiteral("Invalid Alpaca account response")}); return; }
            AccountSnapshot account; account.cash=object.value("cash").toString().toDouble(); account.buyingPower=object.value("buying_power").toString().toDouble(); account.equity=object.value("equity").toString().toDouble();
            handler({true,account,{}});
        }, config_.timeoutMs, authHeaders());
}

AlpacaBrokerClient::Result<Order> AlpacaBrokerClient::parseOrder(const QByteArray& body)
{
    QJsonParseError parseError; const auto object=QJsonDocument::fromJson(body,&parseError).object();
    if (parseError.error != QJsonParseError::NoError || object.value("id").toString().isEmpty()) return {false,{},QStringLiteral("Invalid Alpaca order response")};
    Order order; order.brokerOrderId=object.value("id").toString().toStdString(); order.request.clientOrderId=object.value("client_order_id").toString().toStdString();
    order.request.symbol=object.value("symbol").toString().toStdString(); order.request.side=object.value("side").toString()=="sell"?OrderSide::Sell:OrderSide::Buy;
    order.request.type=object.value("type").toString()=="limit"?OrderType::Limit:OrderType::Market; order.request.quantity=object.value("qty").toString().toLongLong();
    order.request.limitPrice=object.value("limit_price").toString().toDouble(); order.filledQuantity=object.value("filled_qty").toString().toLongLong(); order.averageFillPrice=object.value("filled_avg_price").toString().toDouble();
    order.status=parseStatus(object.value("status").toString()); return {true,order,{}};
}

network::HttpClient::RequestId AlpacaBrokerClient::fetchOrders(QObject* context, OrdersHandler handler)
{
    const QString error=configurationError(false); if(!error.isEmpty()){QMetaObject::invokeMethod(context,[handler=std::move(handler),error]{handler({false,{},error});},Qt::QueuedConnection);return network::HttpClient::InvalidRequestId;}
    return network::HttpClient::instance().getAsync(endpoint(QStringLiteral("/v2/orders?status=all&limit=100")),context,[handler=std::move(handler)](const network::HttpResponse& response){
        if(!response.isSuccess()){handler({false,{},responseError(response)});return;} QJsonParseError error; const auto array=QJsonDocument::fromJson(response.body,&error).array(); if(error.error!=QJsonParseError::NoError){handler({false,{},QStringLiteral("Invalid Alpaca orders response")});return;}
        std::vector<Order> orders; for(const auto& value:array){const auto parsed=parseOrder(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));if(parsed.success)orders.push_back(parsed.value);} handler({true,std::move(orders),{}});
    },config_.timeoutMs,authHeaders());
}

network::HttpClient::RequestId AlpacaBrokerClient::fetchOrder(const QString& brokerOrderId,QObject* context,OrderHandler handler)
{
    const QString error=configurationError(false); if(!error.isEmpty()){QMetaObject::invokeMethod(context,[handler=std::move(handler),error]{handler({false,{},error});},Qt::QueuedConnection);return network::HttpClient::InvalidRequestId;}
    const QString id=QString::fromUtf8(QUrl::toPercentEncoding(brokerOrderId)); return network::HttpClient::instance().getAsync(endpoint("/v2/orders/"+id),context,[handler=std::move(handler)](const network::HttpResponse& response){if(!response.isSuccess()){handler({false,{},responseError(response)});return;}handler(parseOrder(response.body));},config_.timeoutMs,authHeaders());
}

network::HttpClient::RequestId AlpacaBrokerClient::submitOrder(const OrderRequest& request,QObject* context,OrderHandler handler)
{
    const QString error=configurationError(true); if(!error.isEmpty()){QMetaObject::invokeMethod(context,[handler=std::move(handler),error]{handler({false,{},error});},Qt::QueuedConnection);return network::HttpClient::InvalidRequestId;}
    QJsonObject body{{"symbol",QString::fromStdString(request.symbol)},{"qty",QString::number(request.quantity)},{"side",request.side==OrderSide::Buy?"buy":"sell"},{"type",request.type==OrderType::Market?"market":"limit"},{"time_in_force","day"},{"client_order_id",QString::fromStdString(request.clientOrderId)}}; if(request.type==OrderType::Limit)body.insert("limit_price",QString::number(request.limitPrice,'f',2));
    return network::HttpClient::instance().postAsync(endpoint(QStringLiteral("/v2/orders")),QJsonDocument(body).toJson(QJsonDocument::Compact),context,[handler=std::move(handler)](const network::HttpResponse& response){if(!response.isSuccess()){handler({false,{},responseError(response)});return;}handler(parseOrder(response.body));},config_.timeoutMs,authHeaders());
}

network::HttpClient::RequestId AlpacaBrokerClient::cancelOrder(const QString& brokerOrderId,QObject* context,ActionHandler handler)
{
    const QString error=configurationError(true); if(!error.isEmpty()){QMetaObject::invokeMethod(context,[handler=std::move(handler),error]{handler({false,false,error});},Qt::QueuedConnection);return network::HttpClient::InvalidRequestId;}
    const QString id=QString::fromUtf8(QUrl::toPercentEncoding(brokerOrderId)); return network::HttpClient::instance().deleteAsync(endpoint("/v2/orders/"+id),context,[handler=std::move(handler)](const network::HttpResponse& response){handler(response.isSuccess()?Result<bool>{true,true,{}}:Result<bool>{false,false,responseError(response)});},config_.timeoutMs,authHeaders());
}

bool AlpacaBrokerClient::cancelRequest(network::HttpClient::RequestId requestId) { return network::HttpClient::instance().cancel(requestId); }

} // namespace fininsight::trading
