#include "monitoring/HealthMonitor.h"

namespace fininsight::monitoring {

HealthMonitor::HealthMonitor(QObject* parent) : QObject(parent) {}

void HealthMonitor::onStreamState(datahub::QuoteStreamService::State state, const QString& detail)
{
    if (state == datahub::QuoteStreamService::State::Fallback && streamState_ != state) ++fallbackCount_;
    if (state == datahub::QuoteStreamService::State::Disconnected && streamState_ != state) ++disconnectCount_;
    streamState_ = state;
    detail_ = detail;
    emit changed();
}

void HealthMonitor::onQuoteAccepted(qint64 receivedAtMs)
{
    if (receivedAtMs > 0) lastQuoteAtMs_ = receivedAtMs;
    emit changed();
}

void HealthMonitor::onQuoteRejected(const QString& reason)
{
    ++rejectedQuoteCount_;
    detail_ = reason;
    emit changed();
}

HealthMonitor::Snapshot HealthMonitor::snapshot(qint64 nowMs, qint64 staleThresholdMs) const
{
    Snapshot result;
    result.lastQuoteAtMs = lastQuoteAtMs_; result.fallbackCount = fallbackCount_;
    result.disconnectCount = disconnectCount_; result.rejectedQuoteCount = rejectedQuoteCount_; result.detail = detail_;
    using State = datahub::QuoteStreamService::State;
    if (streamState_ == State::Disabled) result.status = Status::Disabled;
    else if (streamState_ == State::Disconnected) result.status = Status::Down;
    else if (streamState_ == State::Fallback || streamState_ == State::Stale) result.status = Status::Degraded;
    else if (lastQuoteAtMs_ > 0 && staleThresholdMs > 0 && nowMs - lastQuoteAtMs_ > staleThresholdMs) result.status = Status::Stale;
    else if (streamState_ == State::Live) result.status = Status::Healthy;
    else result.status = Status::Degraded;
    return result;
}

} // namespace fininsight::monitoring
