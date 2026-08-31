#include "panels/HealthPanel.h"
#include "core/I18n.h"
#include "monitoring/HealthMonitor.h"
#include "monitoring/LatencyTracker.h"
#include <QDateTime>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
namespace fininsight::panels {
HealthPanel::HealthPanel(monitoring::HealthMonitor& health, monitoring::LatencyTracker& latency, QWidget* parent)
    : QWidget(parent), health_(health), latency_(latency)
{
    auto* root = new QVBoxLayout(this); root->setContentsMargins(8,6,8,6);
    status_ = new QLabel; status_->setObjectName(QStringLiteral("healthStatusLabel"));
    counters_ = new QLabel; counters_->setObjectName(QStringLiteral("healthCountersLabel"));
    latencyLabel_ = new QLabel; latencyLabel_->setObjectName(QStringLiteral("healthLatencyLabel"));
    root->addWidget(status_); root->addWidget(counters_); root->addWidget(latencyLabel_); root->addStretch();
    timer_ = new QTimer(this); timer_->setInterval(1000); connect(timer_,&QTimer::timeout,this,&HealthPanel::refresh); timer_->start();
    connect(&health_,&monitoring::HealthMonitor::changed,this,&HealthPanel::refresh); refresh();
}
void HealthPanel::retranslateUi() { refresh(); }
void HealthPanel::refresh()
{
    const auto snapshot=health_.snapshot(QDateTime::currentMSecsSinceEpoch());
    const char* name="Disabled"; using Status=monitoring::HealthMonitor::Status;
    if(snapshot.status==Status::Healthy)name="Healthy";else if(snapshot.status==Status::Degraded)name="Degraded";else if(snapshot.status==Status::Stale)name="Stale";else if(snapshot.status==Status::Down)name="Down";
    status_->setText(I18n::instance().t("Quote stream: %1 | Last tick: %2").arg(I18n::instance().t(QString::fromLatin1(name)), snapshot.lastQuoteAtMs>0?QDateTime::fromMSecsSinceEpoch(snapshot.lastQuoteAtMs).toString(QStringLiteral("HH:mm:ss")):I18n::instance().t("never")));
    counters_->setText(I18n::instance().t("Fallbacks: %1 | Disconnects: %2 | Rejected messages: %3").arg(snapshot.fallbackCount).arg(snapshot.disconnectCount).arg(snapshot.rejectedQuoteCount));
    const auto summary=latency_.summary("quote_delivery"); latencyLabel_->setText(I18n::instance().t("Quote delivery latency (%1): P50 %2 ms | P95 %3 ms | P99 %4 ms").arg(summary.sampleCount).arg(summary.p50Ms,0,'f',1).arg(summary.p95Ms,0,'f',1).arg(summary.p99Ms,0,'f',1));
}
}
