#pragma once

#include "datahub/QuoteData.h"

#include <QObject>

#include <cstdint>
#include <functional>

class QTimer;

namespace fininsight::datahub {

class QuoteStream;

class QuoteStreamService final : public QObject {
    Q_OBJECT

public:
    enum class State { Disabled, Connecting, Live, Fallback, Stale, Disconnected };
    Q_ENUM(State)

    using FallbackHandler = std::function<void(const QString&)>;
    using Clock = std::function<std::int64_t()>;

    explicit QuoteStreamService(QuoteStream* stream,
                                FallbackHandler fallback = {},
                                QObject* parent = nullptr,
                                Clock clock = {});

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    void setSymbol(const QString& symbol);
    QString symbol() const { return symbol_; }
    void setFreshnessThreshold(int thresholdMs);
    State state() const { return state_; }
    std::int64_t lastQuoteAtMs() const { return lastQuoteAtMs_; }

public slots:
    void checkFreshness();

signals:
    void stateChanged(fininsight::datahub::QuoteStreamService::State state,
                      const QString& detail);
    void quoteAccepted(const fininsight::datahub::QuoteData& quote);
    void quoteRejected(const QString& reason);

private:
    void handleQuote(QuoteData quote);
    void handleStreamStatus(const QString& status);
    void degrade(State state, const QString& detail);
    void setState(State state, const QString& detail);
    void requestFallback();
    static QString normalize(const QString& symbol);

    QuoteStream* stream_ = nullptr;
    FallbackHandler fallback_;
    Clock clock_;
    QTimer* freshnessTimer_ = nullptr;
    QString symbol_;
    State state_ = State::Disabled;
    bool enabled_ = false;
    bool fallbackRequested_ = false;
    int freshnessThresholdMs_ = 30000;
    std::int64_t lastQuoteAtMs_ = 0;
    std::int64_t lastSourceTimestampMs_ = 0;
};

} // namespace fininsight::datahub

Q_DECLARE_METATYPE(fininsight::datahub::QuoteStreamService::State)
