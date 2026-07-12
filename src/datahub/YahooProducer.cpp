#include "datahub/YahooProducer.h"
#include "datahub/DataHub.h"
#include "network/HttpClient.h"
#include "storage/StockRepository.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>

namespace fininsight::datahub {

// ── 构造 ────────────────────────────────────────────

YahooProducer::YahooProducer(QObject* parent)
    : QObject(parent)
    , stockRepo_(storage::Database::instance())
{}

// ── URL 构建 ────────────────────────────────────────

QString YahooProducer::buildQuoteUrl(const QString& symbol) {
    return QString("https://query1.finance.yahoo.com/v8/finance/chart/%1"
                   "?interval=2m&range=1d&includePrePost=false")
        .arg(symbol);
}

QString YahooProducer::buildKLineUrl(const QString& symbol,
                                      const QString& range) {
    return QString("https://query1.finance.yahoo.com/v8/finance/chart/%1"
                   "?interval=1d&range=%2&includePrePost=false")
        .arg(symbol, range);
}

// ── 拉取报价 ────────────────────────────────────────

void YahooProducer::fetchQuote(const QString& symbol) {
    const QString url = buildQuoteUrl(symbol);
    qInfo() << "[Yahoo] Fetching quote:" << symbol;

    QByteArray json = fininsight::network::HttpClient::instance().get(url);
    if (json.isEmpty()) {
        qWarning() << "[Yahoo] Empty response for:" << symbol;
        emit errorOccurred(symbol, "Empty response");
        return;
    }

    QuoteData quote = parseQuote(json, symbol);
    if (!quote.isValid()) {
        emit errorOccurred(symbol, "Failed to parse quote");
        return;
    }

    // 写入数据库缓存
    storage::Stock stock;
    stock.symbol    = symbol;
    stock.name      = quote.name;
    stock.exchange  = quote.exchange;
    stock.currency  = quote.currency;
    stock.lastPrice = quote.price;
    stock.updatedAt = QDateTime::currentSecsSinceEpoch();
    auto existing = stockRepo_.findBySymbol(symbol);
    if (existing) {
        stock.id = existing->id;
        stockRepo_.update(stock);
    } else {
        stockRepo_.insert(stock);
    }

    // 发布到 DataHub
    DataHub::instance().publishQuote(quote);
    emit quoteReady(quote);

    qInfo() << "[Yahoo] Quote:" << symbol << quote.price;
}

// ── 拉取 K 线 ───────────────────────────────────────

void YahooProducer::fetchKLine(const QString& symbol, const QString& range) {
    const QString url = buildKLineUrl(symbol, range);
    qInfo() << "[Yahoo] Fetching K-line:" << symbol << "range:" << range;

    QByteArray json = fininsight::network::HttpClient::instance().get(url);
    if (json.isEmpty()) {
        emit errorOccurred(symbol, "Empty K-line response");
        return;
    }

    QVector<KLineData> klines = parseKLine(json, symbol);
    if (klines.isEmpty()) {
        emit errorOccurred(symbol, "Failed to parse K-line data");
        return;
    }

    DataHub::instance().publishKLine(symbol, klines);
    emit klineReady(symbol, klines);

    qInfo() << "[Yahoo] K-line:" << symbol << klines.size() << "bars";
}

// ── 拉取并缓存 ──────────────────────────────────────

void YahooProducer::fetchOrCache(const QString& symbol) {
    auto existing = stockRepo_.findBySymbol(symbol);
    if (existing && existing->updatedAt > 0) {
        // 已有缓存，先发布缓存数据
        QuoteData q;
        q.symbol = existing->symbol;
        q.name   = existing->name;
        q.price  = existing->lastPrice;
        q.exchange = existing->exchange;
        q.currency = existing->currency;
        DataHub::instance().publishQuote(q);
    }
    // 再拉最新数据（异步更新）
    fetchQuote(symbol);
}

// ── JSON 解析 ───────────────────────────────────────

QuoteData YahooProducer::parseQuote(const QByteArray& json,
                                     const QString& symbol) {
    QuoteData q;
    q.symbol = symbol;

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject root = doc.object();
    QJsonObject chart = root["chart"].toObject();
    QJsonArray result = chart["result"].toArray();
    if (result.isEmpty()) return q;

    QJsonObject first = result[0].toObject();
    QJsonObject meta  = first["meta"].toObject();
    q.currency  = meta["currency"].toString();
    q.exchange  = meta["exchangeName"].toString();
    q.price     = meta["regularMarketPrice"].toDouble();
    q.prevClose = meta["previousClose"].toDouble();
    q.open      = meta["regularMarketOpen"].toDouble();
    q.high      = meta["regularMarketDayHigh"].toDouble();
    q.low       = meta["regularMarketDayLow"].toDouble();
    q.volume    = static_cast<qint64>(meta["regularMarketVolume"].toDouble());

    q.change        = q.price - q.prevClose;
    q.changePercent = q.prevClose > 0 ? (q.change / q.prevClose) * 100.0 : 0.0;
    q.timestamp     = QDateTime::currentMSecsSinceEpoch();

    return q;
}

QVector<KLineData> YahooProducer::parseKLine(const QByteArray& json,
                                              const QString& symbol) {
    QVector<KLineData> result;

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject root = doc.object();
    QJsonObject chart = root["chart"].toObject();
    QJsonArray results = chart["result"].toArray();
    if (results.isEmpty()) return result;

    QJsonObject first = results[0].toObject();
    QJsonArray timestamps = first["timestamp"].toArray();
    QJsonObject indicators = first["indicators"].toObject();
    QJsonArray quotes = indicators["quote"].toArray();
    if (quotes.isEmpty()) return result;

    QJsonObject quote = quotes[0].toObject();
    QJsonArray opens  = quote["open"].toArray();
    QJsonArray highs  = quote["high"].toArray();
    QJsonArray lows   = quote["low"].toArray();
    QJsonArray closes = quote["close"].toArray();
    QJsonArray volumes = quote["volume"].toArray();

    for (int i = 0; i < timestamps.size(); ++i) {
        KLineData bar;
        bar.symbol = symbol;
        bar.date   = QDateTime::fromSecsSinceEpoch(
            static_cast<qint64>(timestamps[i].toDouble()))
            .toString("yyyy-MM-dd");
        bar.open   = opens[i].toDouble();
        bar.high   = highs[i].toDouble();
        bar.low    = lows[i].toDouble();
        bar.close  = closes[i].toDouble();
        bar.volume = static_cast<qint64>(volumes[i].toDouble());

        // 跳过空数据
        if (bar.open == 0 && bar.close == 0) continue;
        result.append(bar);
    }

    return result;
}

} // namespace fininsight::datahub
