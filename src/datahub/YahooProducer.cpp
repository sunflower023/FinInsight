#include "datahub/YahooProducer.h"
#include "datahub/QuoteAdapters.h"
#include "datahub/DataHub.h"
#include "network/HttpClient.h"
#include "storage/StockRepository.h"

#include <QDebug>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace fininsight::datahub {

// ── 构造 ────────────────────────────────────────────

YahooProducer::YahooProducer(QObject* parent)
    : QObject(parent)
    , stockRepo_(storage::Database::instance())
{}

// ── URL 构建 ────────────────────────────────────────

QString YahooProducer::buildQuoteUrl(const QString& symbol) {
    return quote_adapters::yahooUrl(symbol);
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
    ++pendingCount_;

    const auto requestId = fininsight::network::HttpClient::instance().getAsync(
        url, this,
        [this, symbol](const fininsight::network::HttpResponse& response) {
            --pendingCount_;
            if (!response.isSuccess()) {
                qWarning() << "[Yahoo] Quote request failed:" << symbol << response.error;
                emit errorOccurred(symbol, response.error);
                return;
            }
            if (response.body.isEmpty()) {
                emit errorOccurred(symbol, "Empty response");
                return;
            }

            QuoteData quote = parseQuote(response.body, symbol);
            if (!quote.isValid()) {
                emit errorOccurred(symbol, "Failed to parse quote");
                return;
            }

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

            DataHub::instance().publishQuote(quote);
            emit quoteReady(quote);

            qInfo() << "[Yahoo] Quote:" << symbol << quote.price;
        });
    if (requestId == fininsight::network::HttpClient::InvalidRequestId) {
        --pendingCount_;
        emit errorOccurred(symbol, "Failed to start HTTP request");
    }
}

// ── 拉取 K 线 ───────────────────────────────────────

void YahooProducer::fetchKLine(const QString& symbol, const QString& range) {
    const QString url = buildKLineUrl(symbol, range);
    qInfo() << "[Yahoo] Fetching K-line:" << symbol << "range:" << range;
    ++pendingCount_;

    const auto requestId = fininsight::network::HttpClient::instance().getAsync(
        url, this,
        [this, symbol](const fininsight::network::HttpResponse& response) {
            --pendingCount_;
            if (!response.isSuccess()) {
                qWarning() << "[Yahoo] K-line request failed:" << symbol << response.error;
                emit errorOccurred(symbol, response.error);
                return;
            }
            if (response.body.isEmpty()) {
                emit errorOccurred(symbol, "Empty K-line response");
                return;
            }

            QVector<KLineData> klines = parseKLine(response.body, symbol);
            if (klines.isEmpty()) {
                emit errorOccurred(symbol, "Failed to parse K-line data");
                return;
            }

            DataHub::instance().publishKLine(symbol, klines);
            emit klineReady(symbol, klines);

            qInfo() << "[Yahoo] K-line:" << symbol << klines.size() << "bars";
        });
    if (requestId == fininsight::network::HttpClient::InvalidRequestId) {
        --pendingCount_;
        emit errorOccurred(symbol, "Failed to start HTTP request");
    }
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
    return quote_adapters::parseYahoo(json, symbol);
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
    QJsonArray adjustedCloses;
    const QJsonArray adjustedSeries = indicators["adjclose"].toArray();
    if (!adjustedSeries.isEmpty()) {
        adjustedCloses = adjustedSeries[0].toObject()["adjclose"].toArray();
    }

    for (int i = 0; i < timestamps.size(); ++i) {
        KLineData bar;
        bar.symbol = symbol;
        bar.date   = QDateTime::fromSecsSinceEpoch(
            static_cast<qint64>(timestamps[i].toDouble()), Qt::UTC)
            .toString("yyyy-MM-dd");
        bar.open   = opens[i].toDouble();
        bar.high   = highs[i].toDouble();
        bar.low    = lows[i].toDouble();
        bar.close  = closes[i].toDouble();
        if (i < adjustedCloses.size() && adjustedCloses[i].isDouble()) {
            bar.adjustedClose = adjustedCloses[i].toDouble();
            bar.hasAdjustedClose = true;
        }
        bar.volume = static_cast<qint64>(volumes[i].toDouble());

        // 跳过空数据
        if (bar.open == 0 && bar.close == 0) continue;
        result.append(bar);
    }

    return result;
}

} // namespace fininsight::datahub
