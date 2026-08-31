#pragma once

#include "network/HttpClient.h"
#include "trading/TradingTypes.h"

#include <QObject>
#include <functional>
#include <vector>

namespace fininsight::trading {

class AlpacaBrokerClient final : public QObject {
    Q_OBJECT
public:
    enum class Environment { Paper, Live };

    struct Config {
        QString baseUrl;
        QString apiKey;
        QString secretKey;
        Environment environment = Environment::Paper;
        bool liveTradingUnlocked = false;
        int timeoutMs = 10000;
    };

    template<typename T> struct Result {
        bool success = false;
        T value{};
        QString error;
    };

    using AccountHandler = std::function<void(const Result<AccountSnapshot>&)>;
    using OrderHandler = std::function<void(const Result<Order>&)>;
    using OrdersHandler = std::function<void(const Result<std::vector<Order>>&)>;
    using ActionHandler = std::function<void(const Result<bool>&)>;

    explicit AlpacaBrokerClient(Config config, QObject* parent = nullptr);

    bool isConfigured() const;
    bool canMutateOrders() const;
    Environment environment() const { return config_.environment; }
    network::HttpClient::RequestId fetchAccount(QObject* context, AccountHandler handler);
    network::HttpClient::RequestId fetchOrders(QObject* context, OrdersHandler handler);
    network::HttpClient::RequestId fetchOrder(const QString& brokerOrderId, QObject* context, OrderHandler handler);
    network::HttpClient::RequestId submitOrder(const OrderRequest& request, QObject* context, OrderHandler handler);
    network::HttpClient::RequestId cancelOrder(const QString& brokerOrderId, QObject* context, ActionHandler handler);
    bool cancelRequest(network::HttpClient::RequestId requestId);

    static QString defaultBaseUrl(Environment environment);

private:
    QMap<QByteArray, QByteArray> authHeaders() const;
    QString endpoint(const QString& path) const;
    QString configurationError(bool mutation) const;
    static Result<Order> parseOrder(const QByteArray& body);

    Config config_;
};

} // namespace fininsight::trading
