#include "datahub/DataHub.h"

#include <QDebug>

namespace fininsight::datahub {

// ── 单例 ────────────────────────────────────────────

DataHub& DataHub::instance() {
    static DataHub hub;
    return hub;
}

// ── 订阅 ────────────────────────────────────────────

int DataHub::subscribe(const QString& topic,
                        std::function<void(const QVariant&)> callback,
                        bool replayLast)
{
    QMutexLocker locker(&mutex_);

    int id = nextId_++;
    subs_.append({id, topic, std::move(callback)});

    // 如果有缓存数据，立即回放（新订阅者能拿到最近一条）
    if (replayLast && lastData_.contains(topic)) {
        const auto& data = lastData_[topic];
        // 在锁外回调，避免死锁
        auto cb = subs_.last().callback;
        locker.unlock();
        cb(data);
    }

    return id;
}

void DataHub::unsubscribe(int id) {
    QMutexLocker locker(&mutex_);
    subs_.erase(std::remove_if(subs_.begin(), subs_.end(),
        [id](const Subscription& s) { return s.id == id; }),
        subs_.end());
}

// ── 发布 ────────────────────────────────────────────

void DataHub::publish(const QString& topic, const QVariant& data) {
    // 先更新缓存
    {
        QMutexLocker locker(&mutex_);
        lastData_[topic] = data;
    }

    // 拷贝一份订阅列表，在锁外回调（避免回调中再调 subscribe/publish 死锁）
    QVector<std::function<void(const QVariant&)>> matchedCallbacks;
    {
        QMutexLocker locker(&mutex_);
        for (const auto& sub : subs_) {
            if (matches(topic, sub.topic)) {
                matchedCallbacks.append(sub.callback);
            }
        }
    }

    for (auto& cb : matchedCallbacks) {
        cb(data);
    }
}

// ── 行情快捷方法 ───────────────────────────────────

void DataHub::publishQuote(const QuoteData& quote) {
    QVariant v;
    v.setValue(quote);
    publish(quote.symbol + ".quote", v);
    emit quotePublished(quote);
}

void DataHub::publishKLine(const QString& symbol, const QVector<KLineData>& klines) {
    QVariant v;
    v.setValue(klines);
    publish(symbol + ".kline.daily", v);
    emit klinePublished(symbol, klines);
}

// ── 状态 ────────────────────────────────────────────

int DataHub::subscriberCount(const QString& topic) const {
    QMutexLocker locker(&mutex_);
    int count = 0;
    for (const auto& sub : subs_) {
        if (matches(topic, sub.topic)) ++count;
    }
    return count;
}

// ── 通配符匹配 ──────────────────────────────────────

bool DataHub::matches(const QString& topic, const QString& pattern) const {
    // 将 glob 风格 "*" 转为 regex ".*"
    QString regexStr = QRegularExpression::wildcardToRegularExpression(pattern);
    QRegularExpression re(regexStr);
    return re.match(topic).hasMatch();
}

} // namespace fininsight::datahub
