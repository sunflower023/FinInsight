#include "datahub/Aggregator.h"
#include "datahub/QuoteAdapters.h"
#include "network/HttpClient.h"

#include <QFile>
#include <QHostAddress>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>

#include <optional>

namespace {

using fininsight::datahub::Aggregator;
using fininsight::datahub::QuoteData;
using fininsight::network::HttpClient;
using fininsight::network::HttpResponse;

QByteArray fixture(const QString& name)
{
    QFile file(QString::fromUtf8(FININSIGHT_FIXTURE_DIR) + '/' + name);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

HttpResponse successfulResponse(const QByteArray& body)
{
    HttpResponse response;
    response.body = body;
    response.statusCode = 200;
    return response;
}

class LocalHttpServer final : public QObject {
public:
    explicit LocalHttpServer(QObject* parent = nullptr)
        : QObject(parent)
    {
        connect(&server_, &QTcpServer::newConnection, this, [this]() {
            while (auto* socket = server_.nextPendingConnection()) {
                socket->setParent(this);
                connect(socket, &QTcpSocket::disconnected,
                        socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
                    QByteArray request = socket->property("request").toByteArray();
                    request += socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n")
                        || socket->property("handled").toBool()) {
                        return;
                    }
                    socket->setProperty("handled", true);

                    const QByteArray path = request.split(' ').value(1);
                    if (path == "/slow") return;

                    const QByteArray body = path == "/ok" ? QByteArray("ok")
                                                           : QByteArray("unavailable");
                    const QByteArray status = path == "/ok"
                        ? QByteArray("200 OK") : QByteArray("503 Service Unavailable");
                    socket->write("HTTP/1.1 " + status + "\r\nContent-Length: "
                                  + QByteArray::number(body.size())
                                  + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    bool listen()
    {
        return server_.listen(QHostAddress::LocalHost, 0);
    }

    QString url(const QString& path) const
    {
        return QStringLiteral("http://127.0.0.1:%1%2")
            .arg(server_.serverPort()).arg(path);
    }

private:
    QTcpServer server_;
};

class FakeTransport final : public QObject {
public:
    struct Plan {
        HttpResponse response;
        int delayMs = 0;
    };

    void setPlan(const QString& source, Plan plan)
    {
        plans_[source] = std::move(plan);
    }

    Aggregator::RequestStarter starter()
    {
        return [this](const QString& url, QObject* context,
                      HttpClient::ResponseHandler handler, int) {
            const auto requestId = nextId_++;
            const QString source = sourceForUrl(url);
            requestedSources_.append(source);
            const Plan plan = plans_.value(source);
            QTimer::singleShot(plan.delayMs, context,
                [this, requestId, handler = std::move(handler), plan]() mutable {
                    completed_.insert(requestId);
                    handler(plan.response);
                });
            return requestId;
        };
    }

    Aggregator::RequestCanceller canceller()
    {
        return [this](HttpClient::RequestId requestId) {
            if (completed_.contains(requestId)) return false;
            cancelled_.insert(requestId);
            return true;
        };
    }

    int requestCount() const { return requestedSources_.size(); }
    int cancellationCount() const { return cancelled_.size(); }

private:
    static QString sourceForUrl(const QString& url)
    {
        if (url.contains(QStringLiteral("eastmoney"))) return QStringLiteral("EastMoney");
        if (url.contains(QStringLiteral("sinajs"))) return QStringLiteral("Sina");
        return QStringLiteral("Yahoo");
    }

    QHash<QString, Plan> plans_;
    QStringList requestedSources_;
    QSet<HttpClient::RequestId> completed_;
    QSet<HttpClient::RequestId> cancelled_;
    HttpClient::RequestId nextId_ = 1;
};

class QtIntegrationTests final : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void quoteAdaptersParseFixtures();
    void httpAsyncSuccessAndHttpError();
    void httpTimeoutAndCancellation();
    void httpContextDestructionSuppressesCallback();
    void aggregatorSelectsFirstValidQuote();
    void aggregatorFallsBackAfterInvalidPayload();
    void aggregatorReportsAllSourcesFailed();
    void aggregatorTimeoutIgnoresLateCallbacks();
};

void QtIntegrationTests::cleanup()
{
    QTRY_COMPARE_WITH_TIMEOUT(HttpClient::instance().activeRequestCount(), 0, 1000);
}

void QtIntegrationTests::quoteAdaptersParseFixtures()
{
    using namespace fininsight::datahub::quote_adapters;

    const auto yahoo = parseYahoo(fixture(QStringLiteral("yahoo_valid.json")),
                                  QStringLiteral("AAPL"));
    QCOMPARE(yahoo.symbol, QStringLiteral("AAPL"));
    QCOMPARE(yahoo.price, 229.15);
    QCOMPARE(yahoo.currency, QStringLiteral("USD"));
    QVERIFY(yahoo.timestamp > 0);
    QVERIFY(parseYahoo(fixture(QStringLiteral("yahoo_missing_price.json")),
                       QStringLiteral("AAPL")).price == 0.0);

    const auto eastMoney = parseEastMoney(
        fixture(QStringLiteral("eastmoney_valid.json")), QStringLiteral("600519"));
    QCOMPARE(eastMoney.price, 1415.0);
    QCOMPARE(eastMoney.exchange, QStringLiteral("SSE"));
    QCOMPARE(eastMoney.volume, 23456);
    QVERIFY(parseEastMoney(fixture(QStringLiteral("eastmoney_null_data.json")),
                           QStringLiteral("600519")).price == 0.0);

    const auto sina = parseSina(fixture(QStringLiteral("sina_valid.txt")),
                                QStringLiteral("600519"));
    QCOMPARE(sina.price, 1415.0);
    QCOMPARE(sina.prevClose, 1406.5);
    QVERIFY(parseSina(fixture(QStringLiteral("sina_malformed.txt")),
                      QStringLiteral("600519")).price == 0.0);
}

void QtIntegrationTests::httpAsyncSuccessAndHttpError()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    QObject context;

    std::optional<HttpResponse> success;
    const auto successId = HttpClient::instance().getAsync(
        server.url(QStringLiteral("/ok")), &context,
        [&success](const HttpResponse& response) { success = response; }, 1000);
    QVERIFY(successId != HttpClient::InvalidRequestId);
    QTRY_VERIFY_WITH_TIMEOUT(success.has_value(), 1000);
    QVERIFY(success->isSuccess());
    QCOMPARE(success->statusCode, 200);
    QCOMPARE(success->body, QByteArray("ok"));

    std::optional<HttpResponse> failure;
    HttpClient::instance().getAsync(
        server.url(QStringLiteral("/status")), &context,
        [&failure](const HttpResponse& response) { failure = response; }, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(failure.has_value(), 1000);
    QVERIFY(!failure->isSuccess());
    QCOMPARE(failure->statusCode, 503);
    QVERIFY(!failure->error.isEmpty());
}

void QtIntegrationTests::httpTimeoutAndCancellation()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    QObject context;

    std::optional<HttpResponse> timeout;
    HttpClient::instance().getAsync(
        server.url(QStringLiteral("/slow")), &context,
        [&timeout](const HttpResponse& response) { timeout = response; }, 40);
    QTRY_VERIFY_WITH_TIMEOUT(timeout.has_value(), 1000);
    QVERIFY(timeout->timedOut);
    QVERIFY(!timeout->cancelled);
    QCOMPARE(timeout->error, QStringLiteral("Request timed out"));

    std::optional<HttpResponse> cancelled;
    const auto requestId = HttpClient::instance().getAsync(
        server.url(QStringLiteral("/slow")), &context,
        [&cancelled](const HttpResponse& response) { cancelled = response; }, 1000);
    QVERIFY(HttpClient::instance().cancel(requestId));
    QTRY_VERIFY_WITH_TIMEOUT(cancelled.has_value(), 1000);
    QVERIFY(cancelled->cancelled);
    QVERIFY(!cancelled->timedOut);
    QCOMPARE(cancelled->error, QStringLiteral("Request cancelled"));
    QVERIFY(!HttpClient::instance().cancel(requestId));
}

void QtIntegrationTests::httpContextDestructionSuppressesCallback()
{
    LocalHttpServer server;
    QVERIFY(server.listen());
    int callbacks = 0;
    auto* context = new QObject;
    HttpClient::instance().getAsync(
        server.url(QStringLiteral("/slow")), context,
        [&callbacks](const HttpResponse&) { ++callbacks; }, 1000);
    delete context;

    QTRY_COMPARE_WITH_TIMEOUT(HttpClient::instance().activeRequestCount(), 0, 1000);
    QTest::qWait(20);
    QCOMPARE(callbacks, 0);
}

void QtIntegrationTests::aggregatorSelectsFirstValidQuote()
{
    FakeTransport transport;
    transport.setPlan(QStringLiteral("EastMoney"),
        {successfulResponse(fixture(QStringLiteral("eastmoney_valid.json"))), 5});
    transport.setPlan(QStringLiteral("Sina"),
        {successfulResponse(fixture(QStringLiteral("sina_valid.txt"))), 80});
    Aggregator aggregator(transport.starter(), transport.canceller());

    int doneCount = 0;
    int resultCount = 0;
    QuoteData selected;
    QSignalSpy errors(&aggregator, &Aggregator::errorOccurred);
    connect(&aggregator, &Aggregator::resultReady, this,
            [&resultCount](const QuoteData&, const QVector<Aggregator::SourceResult>&) {
                ++resultCount;
            });

    aggregator.fetchBest(QStringLiteral("600519"),
        [&doneCount, &selected](const QuoteData& quote) {
            ++doneCount;
            selected = quote;
        }, 500);

    QTRY_COMPARE_WITH_TIMEOUT(doneCount, 1, 1000);
    QTest::qWait(100);
    QCOMPARE(resultCount, 1);
    QCOMPARE(errors.count(), 0);
    QCOMPARE(selected.symbol, QStringLiteral("600519"));
    QCOMPARE(selected.price, 1415.0);
    QCOMPARE(transport.requestCount(), 2);
    QCOMPARE(transport.cancellationCount(), 1);
}

void QtIntegrationTests::aggregatorFallsBackAfterInvalidPayload()
{
    FakeTransport transport;
    transport.setPlan(QStringLiteral("EastMoney"),
        {successfulResponse(fixture(QStringLiteral("eastmoney_null_data.json"))), 5});
    transport.setPlan(QStringLiteral("Sina"),
        {successfulResponse(fixture(QStringLiteral("sina_valid.txt"))), 10});
    Aggregator aggregator(transport.starter(), transport.canceller());

    int doneCount = 0;
    QuoteData selected;
    QSignalSpy errors(&aggregator, &Aggregator::errorOccurred);
    aggregator.fetchBest(QStringLiteral("600519"),
        [&doneCount, &selected](const QuoteData& quote) {
            ++doneCount;
            selected = quote;
        }, 500);

    QTRY_COMPARE_WITH_TIMEOUT(doneCount, 1, 1000);
    QCOMPARE(errors.count(), 0);
    QCOMPARE(selected.price, 1415.0);
    QCOMPARE(selected.exchange, QStringLiteral("SSE"));
    QCOMPARE(transport.cancellationCount(), 0);
}

void QtIntegrationTests::aggregatorReportsAllSourcesFailed()
{
    FakeTransport transport;
    transport.setPlan(QStringLiteral("EastMoney"),
        {successfulResponse(fixture(QStringLiteral("eastmoney_null_data.json"))), 5});
    HttpResponse networkFailure;
    networkFailure.networkError = QNetworkReply::ConnectionRefusedError;
    networkFailure.error = QStringLiteral("Connection refused");
    transport.setPlan(QStringLiteral("Sina"), {networkFailure, 10});
    Aggregator aggregator(transport.starter(), transport.canceller());

    int doneCount = 0;
    QSignalSpy errors(&aggregator, &Aggregator::errorOccurred);
    aggregator.fetchBest(QStringLiteral("600519"),
        [&doneCount](const QuoteData&) { ++doneCount; }, 500);

    QTRY_COMPARE_WITH_TIMEOUT(errors.count(), 1, 1000);
    QCOMPARE(doneCount, 0);
    const QString message = errors.first().at(1).toString();
    QVERIFY(message.contains(QStringLiteral("All data sources failed")));
    QVERIFY(message.contains(QStringLiteral("EastMoney")));
    QVERIFY(message.contains(QStringLiteral("Sina")));
}

void QtIntegrationTests::aggregatorTimeoutIgnoresLateCallbacks()
{
    FakeTransport transport;
    transport.setPlan(QStringLiteral("EastMoney"),
        {successfulResponse(fixture(QStringLiteral("eastmoney_valid.json"))), 80});
    transport.setPlan(QStringLiteral("Sina"),
        {successfulResponse(fixture(QStringLiteral("sina_valid.txt"))), 100});
    Aggregator aggregator(transport.starter(), transport.canceller());

    int doneCount = 0;
    int resultCount = 0;
    QSignalSpy errors(&aggregator, &Aggregator::errorOccurred);
    connect(&aggregator, &Aggregator::resultReady, this,
            [&resultCount](const QuoteData&, const QVector<Aggregator::SourceResult>&) {
                ++resultCount;
            });
    aggregator.fetchBest(QStringLiteral("600519"),
        [&doneCount](const QuoteData&) { ++doneCount; }, 20);

    QTRY_COMPARE_WITH_TIMEOUT(errors.count(), 1, 500);
    QVERIFY(errors.first().at(1).toString().contains(QStringLiteral("timed out")));
    QTest::qWait(130);
    QCOMPARE(doneCount, 0);
    QCOMPARE(resultCount, 0);
    QCOMPARE(errors.count(), 1);
    QCOMPARE(transport.cancellationCount(), 2);
}

} // namespace

QTEST_GUILESS_MAIN(QtIntegrationTests)

#include "qt_tests.moc"
