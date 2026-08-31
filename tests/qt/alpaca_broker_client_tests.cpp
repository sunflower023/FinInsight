#include "trading/AlpacaBrokerClient.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

class AlpacaBrokerClientTests final : public QObject {
    Q_OBJECT
private slots:
    void paperAccountOrderAndCancel();
    void liveMutationsRequireUnlock();
};

class FakeBroker final : public QObject {
public:
    explicit FakeBroker(QObject* parent = nullptr) : QObject(parent)
    {
        connect(&server, &QTcpServer::newConnection, this, [this] {
            auto* socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                buffers[socket] += socket->readAll();
                auto& request = buffers[socket];
                const int headerEnd = request.indexOf("\r\n\r\n");
                if (headerEnd < 0) return;
                int length = 0;
                for (const auto& line : request.left(headerEnd).split('\n'))
                    if (line.toLower().startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
                if (request.size() < headerEnd + 4 + length) return;
                requests.push_back(request);
                const QByteArray firstLine = request.left(request.indexOf("\r\n"));
                QByteArray body; QByteArray status = "200 OK";
                if (firstLine.startsWith("GET /v2/account ")) {
                    body = R"({"cash":"1234.50","buying_power":"2469.00","equity":"1300.25"})";
                } else if (firstLine.startsWith("POST /v2/orders ")) {
                    body = orderBody("new");
                } else if (firstLine.startsWith("GET /v2/orders?")) {
                    body = "[" + orderBody("partially_filled") + "]";
                } else if (firstLine.startsWith("GET /v2/orders/broker-1 ")) {
                    body = orderBody("filled");
                } else if (firstLine.startsWith("DELETE /v2/orders/broker-1 ")) {
                    status = "204 No Content";
                } else {
                    status = "404 Not Found"; body = R"({"message":"not found"})";
                }
                socket->write("HTTP/1.1 " + status + "\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost(); buffers.remove(socket);
            });
        });
    }

    bool listen() { return server.listen(QHostAddress::LocalHost, 0); }
    QString baseUrl() const { return QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()); }
    static QByteArray orderBody(const QByteArray& status)
    {
        return QByteArray(R"({"id":"broker-1","client_order_id":"client-1","symbol":"AAPL","side":"buy","type":"limit","qty":"3","limit_price":"99.50","filled_qty":"1","filled_avg_price":"99.40","status":")") + status + "\"}";
    }

    QTcpServer server;
    QHash<QTcpSocket*, QByteArray> buffers;
    QList<QByteArray> requests;
};

void AlpacaBrokerClientTests::paperAccountOrderAndCancel()
{
    FakeBroker broker; QVERIFY(broker.listen());
    fininsight::trading::AlpacaBrokerClient client({broker.baseUrl(), QStringLiteral("key-id"), QStringLiteral("secret")});
    QVERIFY(client.isConfigured()); QVERIFY(client.canMutateOrders());

    bool accountDone = false;
    client.fetchAccount(this, [&](const auto& result) { QVERIFY(result.success); QCOMPARE(result.value.cash, 1234.5); QCOMPARE(result.value.equity, 1300.25); accountDone = true; });
    QTRY_VERIFY(accountDone);

    fininsight::trading::OrderRequest request; request.clientOrderId="client-1"; request.symbol="AAPL"; request.side=fininsight::trading::OrderSide::Buy; request.type=fininsight::trading::OrderType::Limit; request.quantity=3; request.limitPrice=99.5;
    bool submitDone = false; client.submitOrder(request, this, [&](const auto& result) { QVERIFY(result.success); QCOMPARE(result.value.status, fininsight::trading::OrderStatus::Accepted); QCOMPARE(result.value.brokerOrderId, std::string("broker-1")); submitDone=true; }); QTRY_VERIFY(submitDone);
    bool ordersDone = false; client.fetchOrders(this, [&](const auto& result) { QVERIFY(result.success); QCOMPARE(result.value.size(), std::size_t(1)); QCOMPARE(result.value.front().status, fininsight::trading::OrderStatus::PartiallyFilled); ordersDone=true; }); QTRY_VERIFY(ordersDone);
    bool orderDone = false; client.fetchOrder(QStringLiteral("broker-1"), this, [&](const auto& result) { QVERIFY(result.success); QCOMPARE(result.value.status, fininsight::trading::OrderStatus::Filled); orderDone=true; }); QTRY_VERIFY(orderDone);
    bool cancelDone = false; client.cancelOrder(QStringLiteral("broker-1"), this, [&](const auto& result) { QVERIFY(result.success); QVERIFY(result.value); cancelDone=true; }); QTRY_VERIFY(cancelDone);

    QCOMPARE(broker.requests.size(), 5);
    for (const auto& captured : broker.requests) { const auto lower=captured.toLower(); QVERIFY2(lower.contains("apca-api-key-id: key-id"), captured.constData()); QVERIFY2(lower.contains("apca-api-secret-key: secret"), captured.constData()); }
    QVERIFY(broker.requests.at(1).contains("POST /v2/orders HTTP/1.1")); QVERIFY(broker.requests.at(1).contains("\"client_order_id\":\"client-1\"")); QVERIFY(broker.requests.at(1).contains("\"limit_price\":\"99.50\""));
}

void AlpacaBrokerClientTests::liveMutationsRequireUnlock()
{
    FakeBroker broker; QVERIFY(broker.listen());
    fininsight::trading::AlpacaBrokerClient::Config config{broker.baseUrl(), QStringLiteral("key"), QStringLiteral("secret"), fininsight::trading::AlpacaBrokerClient::Environment::Live, false, 1000};
    fininsight::trading::AlpacaBrokerClient client(config); QVERIFY(client.isConfigured()); QVERIFY(!client.canMutateOrders());
    bool done=false; client.submitOrder({},this,[&](const auto& result){QVERIFY(!result.success);QVERIFY(result.error.contains(QStringLiteral("locked")));done=true;}); QTRY_VERIFY(done); QCOMPARE(broker.requests.size(),0);

    fininsight::trading::AlpacaBrokerClient missing({}); done=false; missing.fetchAccount(this,[&](const auto& result){QVERIFY(!result.success);QVERIFY(result.error.contains(QStringLiteral("not configured")));done=true;}); QTRY_VERIFY(done); QCOMPARE(broker.requests.size(),0);
}

QTEST_GUILESS_MAIN(AlpacaBrokerClientTests)
#include "alpaca_broker_client_tests.moc"
