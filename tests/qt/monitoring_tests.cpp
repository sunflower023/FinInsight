#include "monitoring/HealthMonitor.h"
#include "monitoring/LatencyTracker.h"
#include "panels/HealthPanel.h"
#include <QLabel>
#include <QTest>
#include <limits>
class MonitoringTests final : public QObject {
    Q_OBJECT
private slots:
    void latencyRingAndPercentiles();
    void healthTransitionsAndPanel();
};
void MonitoringTests::latencyRingAndPercentiles(){fininsight::monitoring::LatencyTracker tracker(4);QVERIFY(!tracker.record("",1));QVERIFY(!tracker.record("quote",-1));QVERIFY(!tracker.record("quote",std::numeric_limits<double>::infinity()));for(double v:{1,2,3,4,100})QVERIFY(tracker.record("quote",v));const auto s=tracker.summary("quote");QCOMPARE(s.sampleCount,std::size_t(4));QCOMPARE(s.p50Ms,3.0);QCOMPARE(s.p95Ms,100.0);QCOMPARE(s.p99Ms,100.0);QCOMPARE(s.maxMs,100.0);}
void MonitoringTests::healthTransitionsAndPanel(){using StreamState=fininsight::datahub::QuoteStreamService::State;fininsight::monitoring::HealthMonitor health;fininsight::monitoring::LatencyTracker latency;auto s=health.snapshot(1000);QCOMPARE(s.status,fininsight::monitoring::HealthMonitor::Status::Disabled);health.onStreamState(StreamState::Live,{});health.onQuoteAccepted(1000);QCOMPARE(health.snapshot(1500,1000).status,fininsight::monitoring::HealthMonitor::Status::Healthy);QCOMPARE(health.snapshot(2500,1000).status,fininsight::monitoring::HealthMonitor::Status::Stale);health.onStreamState(StreamState::Fallback,QStringLiteral("fallback"));health.onStreamState(StreamState::Fallback,QStringLiteral("same"));health.onQuoteRejected(QStringLiteral("bad payload"));s=health.snapshot(2600);QCOMPARE(s.status,fininsight::monitoring::HealthMonitor::Status::Degraded);QCOMPARE(s.fallbackCount,1);QCOMPARE(s.rejectedQuoteCount,1);health.onStreamState(StreamState::Disconnected,{});health.onStreamState(StreamState::Disconnected,{});s=health.snapshot(2700);QCOMPARE(s.status,fininsight::monitoring::HealthMonitor::Status::Down);QCOMPARE(s.disconnectCount,1);latency.record("quote_delivery",12);fininsight::panels::HealthPanel panel(health,latency);auto* status=panel.findChild<QLabel*>(QStringLiteral("healthStatusLabel"));auto* counters=panel.findChild<QLabel*>(QStringLiteral("healthCountersLabel"));auto* metric=panel.findChild<QLabel*>(QStringLiteral("healthLatencyLabel"));QVERIFY(status&&counters&&metric);QVERIFY(status->text().contains(QStringLiteral("Down")));QVERIFY(counters->text().contains(QStringLiteral("Disconnects: 1")));QVERIFY(metric->text().contains(QStringLiteral("P50 12.0")));}
QTEST_MAIN(MonitoringTests)
#include "monitoring_tests.moc"
