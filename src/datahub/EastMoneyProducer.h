#pragma once

#include "datahub/QuoteData.h"

#include <QObject>

namespace fininsight::datahub {

/**
 * @brief 东方财富 A 股数据源（免费 HTTP 接口，无需注册）
 *
 * 拉取 A 股（沪市/深市）实时行情，自动发布到 DataHub。
 *
 * secid 规则：
 *   沪市: 1.600519   (前缀 1)
 *   深市: 0.000001   (前缀 0)
 *   创业板: 0.300750
 */
class EastMoneyProducer : public QObject {
    Q_OBJECT

public:
    explicit EastMoneyProducer(QObject* parent = nullptr);

    /// 拉取单只 A 股实时行情
    /// @param symbol 纯数字代码，如 "600519"
    void fetchQuote(const QString& symbol);

    bool isRequesting() const { return pendingCount_ > 0; }

signals:
    void quoteReady(const QuoteData& quote);
    void errorOccurred(const QString& symbol, const QString& message);

private:
    QuoteData parseResponse(const QByteArray& json, const QString& symbol);

    int pendingCount_ = 0;
};

} // namespace fininsight::datahub
