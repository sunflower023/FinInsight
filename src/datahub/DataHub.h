#pragma once

#include "datahub/QuoteData.h"

#include <QObject>
#include <QHash>
#include <QVector>
#include <QVariant>
#include <QMutex>
#include <QRegularExpression>

#include <functional>

namespace fininsight::datahub {

/**
 * @brief 进程内发布订阅数据管道（单例）
 *
 * 面板订阅主题 → 数据源发布数据 → 自动回调通知
 *
 * 主题示例：
 *   "AAPL.quote"        实时报价
 *   "AAPL.kline.daily"  日K线
 *   "*.quote"           所有股票的报价
 */
class DataHub : public QObject {
    Q_OBJECT

public:
    static DataHub& instance();

    // —— 订阅管理 ——
    int subscribe(const QString& topic,
                  std::function<void(const QVariant&)> callback,
                  bool replayLast = true);

    void unsubscribe(int subscriptionId);

    // —— 发布 ——
    void publish(const QString& topic, const QVariant& data);

    // —— 行情快捷方法 ——
    void publishQuote(const QuoteData& quote);
    void publishKLine(const QString& symbol, const QVector<KLineData>& klines);

    // —— 状态 ——
    int subscriberCount(const QString& topic) const;

signals:
    void quotePublished(const QuoteData& quote);
    void klinePublished(const QString& symbol, const QVector<KLineData>& data);

private:
    DataHub() = default;

    struct Subscription {
        int id;
        QString topic;
        std::function<void(const QVariant&)> callback;
    };

    bool matches(const QString& topic, const QString& pattern) const;

    QVector<Subscription> subs_;             // 所有订阅
    QHash<QString, QVariant> lastData_;      // 主题 → 最新一条数据
    mutable QMutex mutex_;
    int nextId_ = 1;
};

} // namespace fininsight::datahub
