#include "datahub/EastMoneyProducer.h"
#include "datahub/QuoteAdapters.h"
#include "datahub/DataHub.h"
#include "network/HttpClient.h"

#include <QDebug>

namespace fininsight::datahub {

// ── 构造 ────────────────────────────────────────────

EastMoneyProducer::EastMoneyProducer(QObject* parent)
    : QObject(parent)
{}

// ── 拉取报价 ────────────────────────────────────────

void EastMoneyProducer::fetchQuote(const QString& symbol) {
    const QString url = quote_adapters::eastMoneyUrl(symbol);

    qInfo() << "[EastMoney] Fetching:" << symbol;
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
    return quote_adapters::parseEastMoney(json, symbol);
}

} // namespace fininsight::datahub
