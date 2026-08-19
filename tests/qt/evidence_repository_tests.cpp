#include "analysis/BehaviorAnalyzer.h"
#include "analysis/ReviewGenerator.h"
#include "storage/Database.h"
#include "storage/EvidenceSnapshotRepository.h"
#include "storage/TradingOrderRepository.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

class EvidenceRepositoryTests final : public QObject {
    Q_OBJECT

private slots:
    void savesSnapshotTradesAndFindings();
};

void EvidenceRepositoryTests::savesSnapshotTradesAndFindings() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto& database = fininsight::storage::Database::instance();
    QVERIFY(database.open(directory.filePath(QStringLiteral("evidence.sqlite"))));

    fininsight::analysis::EvidenceSnapshot evidence;
    evidence.startTimestampMs = 1000;
    evidence.source = "test-source";
    evidence.priceBasis = "Close";
    evidence.endTimestampMs = 2000;
    evidence.maxDrawdown = 0.12;
    evidence.portfolio.initialCash = 10000.0;
    evidence.portfolio.cash = 8000.0;
    evidence.portfolio.holdingsValue = 2300.0;
    evidence.portfolio.totalEquity = 10300.0;
    evidence.portfolio.returnRate = 0.03;
    evidence.trades.push_back({1, fininsight::simulation::TradeSide::Buy,
                               "AAPL", 10, 200.0, 1.0, 1200, 0.0});
    const auto report = fininsight::analysis::analyzeBehavior(evidence);
    fininsight::storage::EvidenceSnapshotRepository repository(database);
    const auto id = repository.save(evidence, report);
    QVERIFY(id.has_value());
    QCOMPARE(repository.count(), std::int64_t(1));

    QSqlQuery version(database.mainConnection());
    QVERIFY(version.exec(QStringLiteral("SELECT MAX(version) FROM schema_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 4);

    fininsight::storage::TradingOrderRepository orderRepository(database);
    fininsight::trading::Order order;
    order.request.clientOrderId = "paper-1";
    order.request.symbol = "AAPL";
    order.request.quantity = 2;
    order.request.timestampMs = 1500;
    order.status = fininsight::trading::OrderStatus::Accepted;
    QVERIFY(orderRepository.save(order));
    order.status = fininsight::trading::OrderStatus::Filled;
    order.filledQuantity = 2;
    order.averageFillPrice = 201.0;
    QVERIFY(orderRepository.save(order));
    const auto savedOrders = orderRepository.recent();
    QCOMPARE(savedOrders.size(), std::size_t(1));
    QCOMPARE(savedOrders.front().status, fininsight::trading::OrderStatus::Filled);
    QCOMPARE(savedOrders.front().averageFillPrice, 201.0);

    QSqlQuery trades(database.mainConnection());
    QVERIFY(trades.exec(QStringLiteral("SELECT symbol, quantity FROM evidence_trades")));
    QVERIFY(trades.next());
    QCOMPARE(trades.value(0).toString(), QStringLiteral("AAPL"));
    QCOMPARE(trades.value(1).toLongLong(), qlonglong(10));
    QSqlQuery metadata(database.mainConnection());
    QVERIFY(metadata.exec(QStringLiteral("SELECT source, price_basis FROM evidence_snapshots")));
    QVERIFY(metadata.next());
    QCOMPARE(metadata.value(0).toString(), QStringLiteral("test-source"));
    QCOMPARE(metadata.value(1).toString(), QStringLiteral("Close"));
    const auto summaries = repository.recent();
    QCOMPARE(summaries.size(), std::size_t(1));
    QCOMPARE(summaries.front().source, std::string("test-source"));
    const auto detail = repository.load(*id);
    QVERIFY(detail.has_value());
    QCOMPARE(detail->trades.size(), std::size_t(1));
    QCOMPARE(detail->trades.front().symbol, std::string("AAPL"));
    QCOMPARE(detail->findings.size(), report.findings.size());
    if (!detail->findings.empty()) {
        QCOMPARE(detail->findings.front().evidenceTradeIds.size(), std::size_t(1));
        QCOMPARE(detail->findings.front().evidenceTradeIds.front(), std::uint64_t(1));
    }
    const auto review = fininsight::analysis::DeterministicReviewGenerator().generate(evidence, report);
    QVERIFY(!review.paragraphs.empty());
    QVERIFY(repository.remove(*id));
    QCOMPARE(repository.count(), std::int64_t(0));
    database.close();
}

QTEST_MAIN(EvidenceRepositoryTests)
#include "evidence_repository_tests.moc"
