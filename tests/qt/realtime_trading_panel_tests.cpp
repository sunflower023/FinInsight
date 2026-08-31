#include "panels/RealtimeTradingPanel.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTest>
class RealtimeTradingPanelTests final : public QObject {
    Q_OBJECT
private slots:
    void paperOrdersAndKillSwitch();
};
void RealtimeTradingPanelTests::paperOrdersAndKillSwitch()
{
    fininsight::panels::RealtimeTradingPanel panel; panel.setCurrentSymbol(QStringLiteral("AAPL"));
    std::vector<fininsight::trading::Order> changes;
    connect(&panel, &fininsight::panels::RealtimeTradingPanel::orderChanged,
            &panel, [&changes](const fininsight::trading::Order& order) { changes.push_back(order); });
    auto* submit = panel.findChild<QPushButton*>(QStringLiteral("tradingSubmitButton"));
    auto* quantity = panel.findChild<QSpinBox*>(QStringLiteral("tradingQuantitySpin"));
    auto* type = panel.findChild<QComboBox*>(QStringLiteral("tradingTypeCombo"));
    auto* limit = panel.findChild<QDoubleSpinBox*>(QStringLiteral("tradingLimitSpin"));
    auto* kill = panel.findChild<QCheckBox*>(QStringLiteral("tradingKillSwitch"));
    auto* account = panel.findChild<QLabel*>(QStringLiteral("tradingAccountLabel"));
    auto* message = panel.findChild<QLabel*>(QStringLiteral("tradingMessageLabel"));
    auto* orders = panel.findChild<QTableWidget*>(QStringLiteral("tradingOrdersTable"));
    QVERIFY(submit && quantity && type && limit && kill && account && message && orders);
    QVERIFY(!submit->isEnabled());
    fininsight::datahub::QuoteData quote; quote.symbol = QStringLiteral("AAPL"); quote.price = 100.0; quote.timestamp = QDateTime::currentMSecsSinceEpoch();
    panel.onQuoteUpdated(quote); QVERIFY(submit->isEnabled()); quantity->setValue(10); submit->click();
    QCOMPARE(orders->rowCount(), 1); QCOMPARE(orders->item(0, 5)->text(), QStringLiteral("Filled"));
    QCOMPARE(changes.size(), std::size_t(1)); QCOMPARE(changes.back().status, fininsight::trading::OrderStatus::Filled);
    QVERIFY(account->text().contains(QStringLiteral("position: 10")));
    kill->setChecked(true); submit->click(); QVERIFY(message->text().contains(QStringLiteral("kill switch"))); QCOMPARE(orders->rowCount(), 1);
    QCOMPARE(changes.size(), std::size_t(2)); QCOMPARE(changes.back().status, fininsight::trading::OrderStatus::RiskRejected);
    kill->setChecked(false); type->setCurrentIndex(1); limit->setValue(90.0); quantity->setValue(1); submit->click();
    QCOMPARE(orders->rowCount(), 2); QCOMPARE(orders->item(1, 5)->text(), QStringLiteral("Accepted"));
    QCOMPARE(changes.back().status, fininsight::trading::OrderStatus::Accepted);
    quote.price = 89.0; quote.timestamp += 1000; panel.onQuoteUpdated(quote); QCOMPARE(orders->item(1, 5)->text(), QStringLiteral("Filled"));
    QCOMPARE(changes.size(), std::size_t(4)); QCOMPARE(changes.back().status, fininsight::trading::OrderStatus::Filled);
}
QTEST_MAIN(RealtimeTradingPanelTests)
#include "realtime_trading_panel_tests.moc"
