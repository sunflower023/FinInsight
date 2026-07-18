#include "datahub/Aggregator.h"
#include "datahub/DataHub.h"
#include "network/HttpClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

namespace fininsight::datahub {
namespace net = fininsight::network;

Aggregator::Aggregator(QObject* parent) : QObject(parent) {}

// ── 竞速择优 ────────────────────────────────────────

void Aggregator::fetchBest(const QString& symbol,
                            std::function<void(QuoteData)> onDone,
                            int timeoutMs)
{
    // 并发发起三路请求
    QFuture<QuoteData> f1 = QtConcurrent::run(
        [this, symbol] { return fetchFromYahoo(symbol); });
    QFuture<QuoteData> f2 = QtConcurrent::run(
        [this, symbol] { return fetchFromEastMoney(symbol); });
    QFuture<QuoteData> f3 = QtConcurrent::run(
        [this, symbol] { return fetchFromSina(symbol); });

    // 简单竞速：先检查哪个有结果
    // （Qt6.7 Concurrent 限制：不做真正的 async race，改为串行取最快有结果的那个）
    QElapsedTimer timer;
    timer.start();

    auto tryGet = [&](QFuture<QuoteData>& f, const QString& src) -> QuoteData {
        if (timer.elapsed() > timeoutMs) return {};
        f.waitForFinished();
        if (timer.elapsed() > timeoutMs) return {};
        auto q = f.result();
        if (!q.isValid()) return {};
        qDebug() << "[Aggregator] Source:" << src << "latency:" << timer.elapsed() << "ms";
        return q;
    };

    // 按优先级：Yahoo > 东方财富 > 新浪
    QuoteData best = tryGet(f1, "Yahoo");
    if (!best.isValid()) best = tryGet(f2, "EastMoney");
    if (!best.isValid()) best = tryGet(f3, "Sina");

    if (best.isValid()) {
        DataHub::instance().publishQuote(best);
        if (onDone) onDone(best);
    }
}

// ── 全量拉取 ────────────────────────────────────────

void Aggregator::fetchAll(const QString& symbol,
                           std::function<void(QuoteData, const QString&)> onEach)
{
    QElapsedTimer t;

    t.start();
    auto y = fetchFromYahoo(symbol);
    if (y.isValid() && onEach) onEach(y, "Yahoo");

    t.restart();
    auto e = fetchFromEastMoney(symbol);
    if (e.isValid() && onEach) onEach(e, "EastMoney");

    t.restart();
    auto s = fetchFromSina(symbol);
    if (s.isValid() && onEach) onEach(s, "Sina");
}

// ── Yahoo ────────────────────────────────────────────

QuoteData Aggregator::fetchFromYahoo(const QString& symbol) {
    QString url = QString("https://query1.finance.yahoo.com/v8/finance/chart/%1"
                          "?interval=2m&range=1d").arg(symbol);
    QByteArray json = net::HttpClient::instance().get(url, 3000);
    if (json.isEmpty()) return {};

    QuoteData q;
    q.symbol = symbol;
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject root = doc.object();
    QJsonObject chart = root["chart"].toObject();
    QJsonArray result = chart["result"].toArray();
    if (result.isEmpty()) return q;

    QJsonObject first = result[0].toObject();
    QJsonObject meta = first["meta"].toObject();
    q.price     = meta["regularMarketPrice"].toDouble();
    q.prevClose = meta["previousClose"].toDouble();
    q.open      = meta["regularMarketOpen"].toDouble();
    q.high      = meta["regularMarketDayHigh"].toDouble();
    q.low       = meta["regularMarketDayLow"].toDouble();
    q.currency  = meta["currency"].toString();
    q.exchange  = meta["exchangeName"].toString();
    q.change        = q.price - q.prevClose;
    q.changePercent = q.prevClose > 0 ? (q.change / q.prevClose) * 100.0 : 0.0;

    return q;
}

QuoteData Aggregator::fetchFromEastMoney(const QString& symbol) {
    // A股用东方财富，非数字代码跳过
    bool ok;
    symbol.toLongLong(&ok);
    if (!ok) return {};

    QString secid = symbol.startsWith('6') ? "1." + symbol : "0." + symbol;
    QString url = "http://push2.eastmoney.com/api/qt/stock/get"
                  "?secid=" + secid +
                  "&fields=f43,f44,f45,f46,f47,f57,f58,f169,f170";

    QByteArray json = net::HttpClient::instance().get(url, 3000);
    if (json.isEmpty()) return {};

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject data = doc.object()["data"].toObject();
    if (data.isEmpty()) return {};

    QuoteData q;
    q.symbol    = symbol;
    q.price     = data["f43"].toDouble() / 100.0;
    q.high      = data["f44"].toDouble() / 100.0;
    q.low       = data["f45"].toDouble() / 100.0;
    q.open      = data["f46"].toDouble() / 100.0;
    q.name      = data["f57"].toString();
    q.change    = data["f58"].toDouble() / 100.0;
    q.changePercent = data["f169"].toDouble() / 100.0;
    q.prevClose = data["f170"].toDouble() / 100.0;
    q.currency  = "CNY";
    q.exchange  = symbol.startsWith('6') ? "SSE" : "SZSE";

    return q;
}

QuoteData Aggregator::fetchFromSina(const QString& symbol) {
    // 新浪财经接口（A股免费）
    bool ok;
    symbol.toLongLong(&ok);
    if (!ok) return {};

    QString code = symbol.startsWith('6') ? "sh" + symbol : "sz" + symbol;
    QString url = "http://hq.sinajs.cn/list=" + code;
    QByteArray bytes = net::HttpClient::instance().get(url, 3000);
    if (bytes.isEmpty()) return {};

    // 新浪返回格式: var hq_str_sh600519="茅台,1500.00,1490.00,..."
    QString resp = QString::fromLocal8Bit(bytes);
    int eq = resp.indexOf('"');
    int end = resp.indexOf('"', eq + 1);
    if (eq < 0 || end < 0) return {};

    QString data = resp.mid(eq + 1, end - eq - 1);
    QStringList parts = data.split(',');

    QuoteData q;
    q.symbol = symbol;
    q.name   = parts.value(0);
    q.open   = parts.value(1).toDouble();
    q.prevClose = parts.value(2).toDouble();
    q.price  = parts.value(3).toDouble();
    q.high   = parts.value(4).toDouble();
    q.low    = parts.value(5).toDouble();
    q.change = q.price - q.prevClose;
    q.changePercent = q.prevClose > 0 ? (q.change / q.prevClose) * 100.0 : 0.0;
    q.currency = "CNY";
    q.exchange = symbol.startsWith('6') ? "SSE" : "SZSE";

    return q;
}

} // namespace fininsight::datahub
