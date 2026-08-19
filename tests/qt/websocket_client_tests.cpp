#include "network/WebSocketClient.h"
#include "datahub/YahooWebSocketAdapter.h"
#include "datahub/QuoteStreamService.h"
#include "datahub/DataHub.h"

#include <QSignalSpy>
#include <QTest>
#include <QWebSocket>
#include <QWebSocketServer>

using fininsight::network::WebSocketClient;

class DataHubSubscription final {
public:
    explicit DataHubSubscription(int id) : id_(id) {}
    ~DataHubSubscription() { fininsight::datahub::DataHub::instance().unsubscribe(id_); }
    DataHubSubscription(const DataHubSubscription&) = delete;
    DataHubSubscription& operator=(const DataHubSubscription&) = delete;
private:
    int id_;
};

class FakeQuoteStream final : public fininsight::datahub::QuoteStream {
public:
    using QuoteStream::QuoteStream;
    void start() override { started = true; emit statusChanged(QStringLiteral("connected")); }
    void stop() override { started = false; }
    void subscribe(const QString& symbol) override { subscribed = symbol; }
    void unsubscribe(const QString& symbol) override { unsubscribed = symbol; }
    bool isConnected() const override { return started; }
    void push(const fininsight::datahub::QuoteData& quote) { emit quoteReceived(quote); }
    void disconnectNow() { started = false; emit statusChanged(QStringLiteral("disconnected")); }

    bool started = false;
    QString subscribed;
    QString unsubscribed;
};

class WebSocketClientTests final : public QObject {
    Q_OBJECT

private slots:
    void connectsAndRelaysText();
    void disconnectStopsReconnect();
    void yahooPricingPublishesRealtimeQuote();
    void streamServiceValidatesFreshnessAndFallback();
};

void WebSocketClientTests::connectsAndRelaysText() {
    QWebSocketServer server(QStringLiteral("fixture"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QWebSocket* peer = nullptr;
    connect(&server, &QWebSocketServer::newConnection, this, [&] {
        peer = server.nextPendingConnection();
        connect(peer, &QWebSocket::textMessageReceived, peer,
                [peer](const QString& message) { peer->sendTextMessage("echo:" + message); });
    });

    WebSocketClient client;
    client.setReconnectPolicy(false);
    client.setHeartbeatInterval(0);
    QSignalSpy connected(&client, &WebSocketClient::connected);
    QSignalSpy received(&client, &WebSocketClient::textMessageReceived);
    QSignalSpy disconnected(&client, &WebSocketClient::disconnected);

    client.connectToUrl(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));
    QTRY_COMPARE_WITH_TIMEOUT(connected.count(), 1, 2000);
    QVERIFY(peer != nullptr);

    client.sendText(QStringLiteral("hello"));
    QTRY_COMPARE_WITH_TIMEOUT(received.count(), 1, 2000);
    QCOMPARE(received.at(0).at(0).toString(), QStringLiteral("echo:hello"));

    client.disconnectFromServer();
    QTRY_COMPARE_WITH_TIMEOUT(disconnected.count(), 1, 2000);
    QVERIFY(!client.isConnected());
    server.close();
}

void WebSocketClientTests::disconnectStopsReconnect() {
    QWebSocketServer server(QStringLiteral("fixture"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    WebSocketClient client;
    client.setReconnectPolicy(true, 10, 20, -1);
    QSignalSpy scheduled(&client, &WebSocketClient::reconnectScheduled);
    client.connectToUrl(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort())));
    QTRY_VERIFY_WITH_TIMEOUT(client.isConnected(), 2000);
    client.disconnectFromServer();
    QTest::qWait(80);
    QCOMPARE(scheduled.count(), 0);
    server.close();
}

void WebSocketClientTests::yahooPricingPublishesRealtimeQuote() {
    QWebSocketServer server(QStringLiteral("yahoo-fixture"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QWebSocket* peer = nullptr;
    QString subscription;
    connect(&server, &QWebSocketServer::newConnection, this, [&] {
        peer = server.nextPendingConnection();
        connect(peer, &QWebSocket::textMessageReceived, this,
                [&](const QString& message) { subscription = message; });
    });

    const QUrl endpoint(QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort()));
    fininsight::datahub::YahooWebSocketAdapter adapter(endpoint);
    int fallbacks = 0;
    fininsight::datahub::QuoteStreamService service(
        &adapter, [&](const QString&) { ++fallbacks; });
    QSignalSpy quotes(&adapter, &fininsight::datahub::YahooWebSocketAdapter::quoteReceived);
    fininsight::datahub::QuoteData published;
    const int subId = fininsight::datahub::DataHub::instance().subscribe(
        QStringLiteral("AAPL.quote.realtime"),
        [&](const QVariant& value) { published = value.value<fininsight::datahub::QuoteData>(); }, false);
    DataHubSubscription subscriptionGuard(subId);

    service.setSymbol(QStringLiteral("AAPL"));
    service.setEnabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(peer != nullptr, 2000);
    QTRY_VERIFY_WITH_TIMEOUT(!subscription.isEmpty(), 2000);
    QByteArray pricing;
    pricing.append(char(0x0a)); pricing.append(char(0x04)); pricing.append("AAPL");
    pricing.append(char(0x11)); double price = 190.5; pricing.append(reinterpret_cast<const char*>(&price), 8);
    pricing.append(char(0x19)); double changePct = 1.25; pricing.append(reinterpret_cast<const char*>(&changePct), 8);
    pricing.append(char(0x21)); double high = 192.0; pricing.append(reinterpret_cast<const char*>(&high), 8);
    pricing.append(char(0x29)); double low = 187.0; pricing.append(reinterpret_cast<const char*>(&low), 8);
    pricing.append(char(0x61)); double change = 2.35; pricing.append(reinterpret_cast<const char*>(&change), 8);
    pricing.append(char(0x18)); pricing.append(char(0x8b)); pricing.append(char(0x9a)); pricing.append(char(0x88)); pricing.append(char(0x8c)); pricing.append(char(0x32));
    peer->sendBinaryMessage(pricing);
    QTRY_COMPARE_WITH_TIMEOUT(quotes.count(), 1, 2000);
    QCOMPARE(published.symbol, QStringLiteral("AAPL"));
    QCOMPARE(published.price, 190.5);
    QCOMPARE(published.change, 2.35);
    QCOMPARE(fallbacks, 0);

    service.setEnabled(false);
    server.close();
}

void WebSocketClientTests::streamServiceValidatesFreshnessAndFallback() {
    FakeQuoteStream stream;
    std::int64_t now = 10000;
    int fallbackCount = 0;
    fininsight::datahub::QuoteStreamService service(
        &stream, [&](const QString& symbol) {
            QCOMPARE(symbol, QStringLiteral("AAPL"));
            ++fallbackCount;
        }, nullptr, [&] { return now; });
    service.setFreshnessThreshold(500);
    QSignalSpy accepted(&service, &fininsight::datahub::QuoteStreamService::quoteAccepted);
    QSignalSpy rejected(&service, &fininsight::datahub::QuoteStreamService::quoteRejected);

    service.setSymbol(QStringLiteral(" aapl "));
    service.setEnabled(true);
    QCOMPARE(stream.subscribed, QStringLiteral("AAPL"));

    fininsight::datahub::QuoteData wrong;
    wrong.symbol = QStringLiteral("MSFT");
    wrong.price = 100.0;
    wrong.timestamp = 10;
    stream.push(wrong);
    QCOMPARE(rejected.count(), 1);

    fininsight::datahub::QuoteData valid;
    valid.symbol = QStringLiteral("AAPL");
    valid.price = 190.0;
    valid.timestamp = 10;
    stream.push(valid);
    QCOMPARE(accepted.count(), 1);
    QCOMPARE(service.state(), fininsight::datahub::QuoteStreamService::State::Live);

    valid.timestamp = 9;
    stream.push(valid);
    QCOMPARE(rejected.count(), 2);
    now += 600;
    service.checkFreshness();
    QCOMPARE(service.state(), fininsight::datahub::QuoteStreamService::State::Stale);
    QCOMPARE(fallbackCount, 1);
    stream.disconnectNow();
    QCOMPARE(fallbackCount, 1);

    service.setEnabled(false);
    QCOMPARE(stream.unsubscribed, QStringLiteral("AAPL"));
}

QTEST_MAIN(WebSocketClientTests)
#include "websocket_client_tests.moc"
