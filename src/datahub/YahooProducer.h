#pragma once

#include "datahub/QuoteData.h"
#include "storage/StockRepository.h"

#include <QObject>
#include <QVector>

namespace fininsight::datahub {

/**
 * @brief Yahoo Finance 数据源（美股免费接口，无需注册）
 *
 * 拉取实时报价和历史 K 线，自动发布到 DataHub 并写入 SQLite 缓存。
 */
class YahooProducer : public QObject {
    Q_OBJECT

public:
    explicit YahooProducer(QObject* parent = nullptr);

    /// 拉取单只股票实时报价，完成后发布到 DataHub
    void fetchQuote(const QString& symbol);

    /// 拉取历史日 K 线（range: "1mo"/"3mo"/"6mo"/"1y"/"5y"）
    void fetchKLine(const QString& symbol,
                    const QString& range = "6mo");

    /// 拉取并缓存：先查 SQLite，没有才调 API
    void fetchOrCache(const QString& symbol);

    // —— 状态 ——
    bool isRequesting() const { return pendingCount_ > 0; }

signals:
    void quoteReady(const QuoteData& quote);
    void klineReady(const QString& symbol, const QVector<KLineData>& data);
    void errorOccurred(const QString& symbol, const QString& message);

private:
    QuoteData parseQuote(const QByteArray& json, const QString& symbol);
    QVector<KLineData> parseKLine(const QByteArray& json, const QString& symbol);

    QString buildQuoteUrl(const QString& symbol);
    QString buildKLineUrl(const QString& symbol, const QString& range);

    storage::StockRepository stockRepo_;
    int pendingCount_ = 0;
};

} // namespace fininsight::datahub
