#pragma once

#include "datahub/QuoteData.h"

#include <QObject>
#include <QFuture>
#include <functional>

namespace fininsight::datahub {

/**
 * @brief 多源行情聚合器 — 并发拉取多个数据源，竞速择优
 *
 * 策略：
 *   1. 同时向 Yahoo + 东方财富 + 新浪 发起请求
 *   2. 最先返回且数据合法的视为最佳结果
 *   3. 超时 5s 未返回 → 降级到备选源
 */
class Aggregator : public QObject {
    Q_OBJECT

public:
    explicit Aggregator(QObject* parent = nullptr);

    /// 并发拉取多路数据，返回第一份有效结果
    /// @param symbol    股票代码
    /// @param onDone    回调（最优 QuoteData）
    /// @param timoutMs  总超时（默认 5s）
    void fetchBest(const QString& symbol,
                   std::function<void(QuoteData)> onDone,
                   int timeoutMs = 5000);

    /// 并发拉取，每路结果都回调（用于日志/对比）
    void fetchAll(const QString& symbol,
                  std::function<void(QuoteData, const QString& source)> onEach);

    /// 记录：源名称 + 延迟 + 数据完整性
    struct SourceResult {
        QString source;     // "Yahoo" / "EastMoney" / "Sina"
        int     latencyMs;
        QuoteData data;
        bool    valid;
    };

signals:
    void resultReady(const QuoteData& best, const QVector<SourceResult>& all);

private:
    QuoteData fetchFromYahoo(const QString& symbol);
    QuoteData fetchFromEastMoney(const QString& symbol);
    QuoteData fetchFromSina(const QString& symbol);
};

} // namespace fininsight::datahub
