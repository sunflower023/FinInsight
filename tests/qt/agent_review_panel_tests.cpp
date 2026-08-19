#include "analysis/BehaviorAnalyzer.h"
#include "panels/AgentReviewPanel.h"
#include "storage/Database.h"
#include "storage/EvidenceSnapshotRepository.h"

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>

class AgentReviewPanelTests final : public QObject {
    Q_OBJECT

private slots:
    void loadsEvidenceAndHighlightsFinding();
};

void AgentReviewPanelTests::loadsEvidenceAndHighlightsFinding()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto& database = fininsight::storage::Database::instance();
    QVERIFY(database.open(directory.filePath(QStringLiteral("agent-review.sqlite"))));

    fininsight::analysis::EvidenceSnapshot evidence;
    evidence.source = "panel-test";
    evidence.priceBasis = "Close";
    evidence.startTimestampMs = 1000;
    evidence.endTimestampMs = 86401000;
    evidence.portfolio.totalEquity = 10200.0;
    evidence.portfolio.returnRate = 0.02;
    evidence.trades = {
        {1, fininsight::simulation::TradeSide::Buy, "AAPL", 10, 100.0, 0.0, 1000, 0.0},
        {2, fininsight::simulation::TradeSide::Buy, "MSFT", 1, 100.0, 0.0, 2000, 0.0},
    };
    const auto report = fininsight::analysis::analyzeBehavior(evidence);
    QVERIFY(!report.findings.empty());
    fininsight::storage::EvidenceSnapshotRepository repository(database);
    QVERIFY(repository.save(evidence, report).has_value());

    fininsight::panels::AgentReviewPanel panel(repository);
    auto* selector = panel.findChild<QComboBox*>(QStringLiteral("agentSnapshotSelector"));
    auto* summary = panel.findChild<QLabel*>(QStringLiteral("agentSummaryLabel"));
    auto* review = panel.findChild<QLabel*>(QStringLiteral("agentReviewTextLabel"));
    auto* findings = panel.findChild<QListWidget*>(QStringLiteral("agentFindingsList"));
    auto* trades = panel.findChild<QTableWidget*>(QStringLiteral("agentTradesTable"));
    QVERIFY(selector && summary && review && findings && trades);
    QCOMPARE(selector->count(), 1);
    QVERIFY(summary->text().contains(QStringLiteral("panel-test")));
    QVERIFY(review->text().contains(QStringLiteral("Evidence covers")));
    QVERIFY(findings->count() > 0);
    QCOMPARE(trades->rowCount(), 2);

    findings->setCurrentRow(0);
    QTest::mouseClick(findings->viewport(), Qt::LeftButton,
                      Qt::NoModifier, findings->visualItemRect(findings->item(0)).center());
    QVERIFY(trades->selectionModel()->selectedRows().size() > 0);
    database.close();
}

QTEST_MAIN(AgentReviewPanelTests)
#include "agent_review_panel_tests.moc"
