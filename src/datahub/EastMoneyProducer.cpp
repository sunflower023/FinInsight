#include "datahub/EastMoneyProducer.h"
#include "datahub/DataHub.h"
#include "network/HttpClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

namespace fininsight::datahub {

// ── 构造 ────────────────────────────────────────────

EastMoneyProducer::EastMoneyProducer(QObject* parent)
    : QObject(parent)
{}

// ── 证券 ID 构建 ────────────────────────────────────

QString EastMoneyProducer::buildSecId(const QString& symbol) {
    // 沪市: 6xxxxx  → 1.600519
    // 深市: 0xxxxx/3xxxxx  → 0.000001 / 0.300750
    if (symbol.startsWith('6')) {
        return "1." + symbol;
    }
    return "0." + symbol;
}

QString EastMoneyProducer::buildUrl(const QString& secid) {
    // 东方财富免费行情接口，无需 API Key
    return QString("http://push2.eastmoney.com/api/qt/stock/get"
                   "?secid=%1"
                   "&fields=f43,f44,f45,f46,f47,f48,f57,f58,f170,f169,f116,f117")
        .arg(secid);
}

// ── 拉取报价 ────────────────────────────────────────

void EastMoneyProducer::fetchQuote(const QString& symbol) {
    const QString secid = buildSecId(symbol);
    const QString url   = buildUrl(secid);

    qInfo() << "[EastMoney] Fetching:" << symbol << "secid:" << secid;
    ++pendingCount_;

    const auto requestId = fininsight::network::HttpClient::instance().getAsync(
        url, this,
        [this, symbol](const fininsight::network::HttpResponse& response) {
            --pendingCount_;
            if (!response.isSuccess()) {
                qWarning() << "[EastMoney] Request failed:" << symbol << response.error;
                emit errorOccurred(symbol, response.error);
                return;
            }
            if (response.body.isEmpty()) {
                emit errorOccurred(symbol, "Empty response");
                return;
            }

            QuoteData quote = parseResponse(response.body, symbol);
            if (!quote.isValid()) {
                emit errorOccurred(symbol, "Parse failed");
                return;
            }

            DataHub::instance().publishQuote(quote);
            emit quoteReady(quote);

            qInfo() << "[EastMoney]" << symbol << quote.name
                    << quote.price << quote.changePercent << "%";
        });
    if (requestId == fininsight::network::HttpClient::InvalidRequestId) {
        --pendingCount_;
        emit errorOccurred(symbol, "Failed to start HTTP request");
    }
}

// ── JSON 解析 ───────────────────────────────────────

QuoteData EastMoneyProducer::parseResponse(const QByteArray& json,
                                            const QString& symbol) {
    QuoteData q;
    q.symbol = symbol;

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject root = doc.object();
    QJsonObject data = root["data"].toObject();
    if (data.isEmpty()) return q;

    // f43=最新价, f44=最高, f45=最低, f46=今开, f47=总手(volume)
    // f48=换手率, f57=名称, f58=涨跌额, f169=涨跌幅, f170=昨收
    q.price        = data["f43"].toDouble() / 100.0;
    q.high         = data["f44"].toDouble() / 100.0;
    q.low          = data["f45"].toDouble() / 100.0;
    q.open         = data["f46"].toDouble() / 100.0;
    q.volume       = static_cast<qint64>(data["f47"].toDouble());
    q.name         = data["f57"].toString();
    q.change       = data["f58"].toDouble() / 100.0;
    q.changePercent = data["f169"].toDouble() / 100.0;
    q.prevClose    = data["f170"].toDouble() / 100.0;

    q.currency  = "CNY";
    q.exchange  = symbol.startsWith('6') ? "SSE" : "SZSE";
    q.timestamp = QDateTime::currentMSecsSinceEpoch();

    return q;
}

} // namespace fininsight::datahub
