#pragma once

#include "datahub/QuoteStreamService.h"
#include <QObject>

namespace fininsight::monitoring {

class HealthMonitor final : public QObject {
    Q_OBJECT
public:
    enum class Status { Disabled, Healthy, Degraded, Stale, Down };
    Q_ENUM(Status)

    struct Snapshot {
        Status status = Status::Disabled;
        qint64 lastQuoteAtMs = 0;
        int fallbackCount = 0;
        int disconnectCount = 0;
        int rejectedQuoteCount = 0;
        QString detail;
    };

    explicit HealthMonitor(QObject* parent = nullptr);
    void onStreamState(datahub::QuoteStreamService::State state, const QString& detail);
    void onQuoteAccepted(qint64 receivedAtMs);
    void onQuoteRejected(const QString& reason);
    Snapshot snapshot(qint64 nowMs, qint64 staleThresholdMs = 30000) const;

signals:
    void changed();

private:
    datahub::QuoteStreamService::State streamState_ = datahub::QuoteStreamService::State::Disabled;
    qint64 lastQuoteAtMs_ = 0;
    int fallbackCount_ = 0;
    int disconnectCount_ = 0;
    int rejectedQuoteCount_ = 0;
    QString detail_;
};

} // namespace fininsight::monitoring
