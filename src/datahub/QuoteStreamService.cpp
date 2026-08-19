#include "datahub/QuoteStreamService.h"

#include "datahub/DataHub.h"
#include "datahub/QuoteStream.h"

#include <QDateTime>
#include <QTimer>

#include <cmath>

namespace fininsight::datahub {

QuoteStreamService::QuoteStreamService(QuoteStream* stream,
                                       FallbackHandler fallback,
                                       QObject* parent,
                                       Clock clock)
    : QObject(parent)
    , stream_(stream)
    , fallback_(std::move(fallback))
    , clock_(clock ? std::move(clock) : [] { return QDateTime::currentMSecsSinceEpoch(); })
    , freshnessTimer_(new QTimer(this))
{
    Q_ASSERT(stream_);
    freshnessTimer_->setInterval(1000);
    connect(freshnessTimer_, &QTimer::timeout, this, &QuoteStreamService::checkFreshness);
    connect(stream_, &QuoteStream::quoteReceived, this, &QuoteStreamService::handleQuote);
    connect(stream_, &QuoteStream::statusChanged, this, &QuoteStreamService::handleStreamStatus);
    connect(stream_, &QuoteStream::errorOccurred, this, [this](const QString& error) {
        degrade(State::Fallback, error);
    });
}

void QuoteStreamService::setEnabled(bool enabled)
{
    if (enabled_ == enabled) return;
    enabled_ = enabled;
    fallbackRequested_ = false;
    lastQuoteAtMs_ = 0;
    lastSourceTimestampMs_ = 0;
    if (!enabled_) {
        freshnessTimer_->stop();
        if (!symbol_.isEmpty()) stream_->unsubscribe(symbol_);
        stream_->stop();
        setState(State::Disabled, QStringLiteral("Experimental stream disabled"));
        return;
    }
    setState(State::Connecting, QStringLiteral("Connecting experimental stream"));
    if (!symbol_.isEmpty()) stream_->subscribe(symbol_);
    stream_->start();
    freshnessTimer_->start();
}

void QuoteStreamService::setSymbol(const QString& symbol)
{
    const QString value = normalize(symbol);
    if (value == symbol_) return;
    if (enabled_ && !symbol_.isEmpty()) stream_->unsubscribe(symbol_);
    symbol_ = value;
    fallbackRequested_ = false;
    lastQuoteAtMs_ = 0;
    lastSourceTimestampMs_ = 0;
    if (enabled_ && !symbol_.isEmpty()) {
        stream_->subscribe(symbol_);
        setState(State::Connecting, QStringLiteral("Waiting for %1").arg(symbol_));
    }
}

void QuoteStreamService::setFreshnessThreshold(int thresholdMs)
{
    freshnessThresholdMs_ = qMax(100, thresholdMs);
}

void QuoteStreamService::checkFreshness()
{
    if (!enabled_ || symbol_.isEmpty() || lastQuoteAtMs_ == 0) return;
    if (clock_() - lastQuoteAtMs_ > freshnessThresholdMs_) {
        degrade(State::Stale, QStringLiteral("Realtime quote is stale"));
    }
}

void QuoteStreamService::handleQuote(QuoteData quote)
{
    if (!enabled_) return;
    quote.symbol = normalize(quote.symbol);
    if (quote.symbol != symbol_) {
        emit quoteRejected(QStringLiteral("Unexpected symbol: %1").arg(quote.symbol));
        return;
    }
    if (!std::isfinite(quote.price) || quote.price <= 0.0 || quote.timestamp <= 0) {
        emit quoteRejected(QStringLiteral("Invalid realtime quote"));
        return;
    }
    if (quote.timestamp < 100000000000LL) quote.timestamp *= 1000;
    if (lastSourceTimestampMs_ > 0 && quote.timestamp < lastSourceTimestampMs_) {
        emit quoteRejected(QStringLiteral("Out-of-order realtime quote"));
        return;
    }
    lastSourceTimestampMs_ = quote.timestamp;
    lastQuoteAtMs_ = clock_();
    fallbackRequested_ = false;
    DataHub::instance().publishRealtimeQuote(quote);
    DataHub::instance().publishQuote(quote);
    setState(State::Live, QStringLiteral("Experimental live feed"));
    emit quoteAccepted(quote);
}

void QuoteStreamService::handleStreamStatus(const QString& status)
{
    if (!enabled_) return;
    if (status == QStringLiteral("connected")) {
        setState(State::Connecting, QStringLiteral("Connected; waiting for quote"));
    } else if (status == QStringLiteral("connecting")) {
        setState(State::Connecting, QStringLiteral("Connecting experimental stream"));
    } else if (status == QStringLiteral("disconnected")) {
        degrade(State::Disconnected, QStringLiteral("Realtime stream disconnected"));
    }
}

void QuoteStreamService::degrade(State state, const QString& detail)
{
    setState(state, detail);
    requestFallback();
}

void QuoteStreamService::setState(State state, const QString& detail)
{
    if (state_ == state && detail.isEmpty()) return;
    state_ = state;
    emit stateChanged(state_, detail);
}

void QuoteStreamService::requestFallback()
{
    if (fallbackRequested_ || symbol_.isEmpty() || !fallback_) return;
    fallbackRequested_ = true;
    fallback_(symbol_);
}

QString QuoteStreamService::normalize(const QString& symbol)
{
    return symbol.trimmed().toUpper();
}

} // namespace fininsight::datahub
