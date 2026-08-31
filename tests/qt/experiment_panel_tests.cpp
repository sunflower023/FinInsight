#include "panels/ExperimentPanel.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTest>

namespace {

using fininsight::datahub::KLineData;
using fininsight::panels::ExperimentPanel;

template <typename T>
T* child(ExperimentPanel& panel, const char* objectName)
{
    auto* result = panel.findChild<T*>(QString::fromLatin1(objectName));
    Q_ASSERT(result);
    return result;
}

KLineData bar(const QString& date, double close, double adjustedClose)
{
    KLineData result;
    result.symbol = QStringLiteral("AAPL");
    result.date = date;
    result.close = close;
    result.adjustedClose = adjustedClose;
    result.hasAdjustedClose = true;
    return result;
}

QVector<KLineData> validBars()
{
    return {
        bar(QStringLiteral("2024-01-02"), 100.0, 90.0),
        bar(QStringLiteral("2024-01-03"), 120.0, 110.0),
        bar(QStringLiteral("2024-01-04"), 90.0, 85.0),
    };
}

class ExperimentPanelTests final : public QObject {
    Q_OBJECT

private slots:
    void requiresMatchingValidatedHistory();
    void runsCloseAndAdjustedExperiments();
    void reportsInputAndPriceConventionErrors();
};

void ExperimentPanelTests::requiresMatchingValidatedHistory()
{
    ExperimentPanel panel;
    auto* run = child<QPushButton>(panel, "experimentRunButton");
    auto* data = child<QLabel>(panel, "experimentDataLabel");
    QVERIFY(!run->isEnabled());

    panel.setCurrentSymbol(QStringLiteral(" aapl "));
    QVERIFY(!run->isEnabled());
    QVERIFY(data->text().contains(QStringLiteral("AAPL")));

    panel.setHistoricalData(QStringLiteral("MSFT"), validBars());
    QVERIFY(!run->isEnabled());
    panel.setHistoricalData(QStringLiteral("AAPL"), validBars());
    QVERIFY(run->isEnabled());
    QVERIFY(data->text().contains(QStringLiteral("3 daily bars")));
    QCOMPARE(child<QDateEdit>(panel, "experimentStartDate")->date(),
             QDate(2024, 1, 2));
    QCOMPARE(child<QDateEdit>(panel, "experimentEndDate")->date(),
             QDate(2024, 1, 4));

    panel.setCurrentSymbol(QStringLiteral("MSFT"));
    QVERIFY(!run->isEnabled());
    QCOMPARE(child<QLabel>(panel, "experimentExecutionLabel")->text(),
             QStringLiteral("--"));
}

void ExperimentPanelTests::runsCloseAndAdjustedExperiments()
{
    ExperimentPanel panel;
    panel.setCurrentSymbol(QStringLiteral("AAPL"));
    panel.setHistoricalData(QStringLiteral("AAPL"), validBars());

    child<QDoubleSpinBox>(panel, "experimentInitialCash")->setValue(1000.0);
    child<QDoubleSpinBox>(panel, "experimentFee")->setValue(5.0);
    auto* run = child<QPushButton>(panel, "experimentRunButton");
    run->click();

    QCOMPARE(child<QLabel>(panel, "experimentExecutionLabel")->text(),
             QStringLiteral("2024-01-02 at $100.00"));
    QCOMPARE(child<QLabel>(panel, "experimentEndingLabel")->text(),
             QStringLiteral("2024-01-04 at $90.00"));
    QCOMPARE(child<QLabel>(panel, "experimentQuantityLabel")->text(),
             QStringLiteral("9"));
    QCOMPARE(child<QLabel>(panel, "experimentEndingCashLabel")->text(),
             QStringLiteral("$95.00"));
    QCOMPARE(child<QLabel>(panel, "experimentMarketValueLabel")->text(),
             QStringLiteral("$810.00"));
    QCOMPARE(child<QLabel>(panel, "experimentEquityLabel")->text(),
             QStringLiteral("$905.00"));
    QCOMPARE(child<QLabel>(panel, "experimentPnlLabel")->text(),
             QStringLiteral("-$95.00"));
    QCOMPARE(child<QLabel>(panel, "experimentReturnLabel")->text(),
             QStringLiteral("-9.50%"));
    QCOMPARE(child<QLabel>(panel, "experimentDrawdownLabel")->text(),
             QStringLiteral("22.98%"));

    auto* priceField = child<QComboBox>(panel, "experimentPriceField");
    priceField->setCurrentIndex(1);
    run->click();
    QCOMPARE(child<QLabel>(panel, "experimentExecutionLabel")->text(),
             QStringLiteral("2024-01-02 at $90.00"));
    QCOMPARE(child<QLabel>(panel, "experimentQuantityLabel")->text(),
             QStringLiteral("11"));
    QCOMPARE(child<QLabel>(panel, "experimentEquityLabel")->text(),
             QStringLiteral("$940.00"));
    QCOMPARE(child<QLabel>(panel, "experimentReturnLabel")->text(),
             QStringLiteral("-6.00%"));
    QCOMPARE(child<QLabel>(panel, "experimentDrawdownLabel")->text(),
             QStringLiteral("22.63%"));
}

void ExperimentPanelTests::reportsInputAndPriceConventionErrors()
{
    ExperimentPanel panel;
    auto bars = validBars();
    bars[1].hasAdjustedClose = false;
    panel.setCurrentSymbol(QStringLiteral("AAPL"));
    panel.setHistoricalData(QStringLiteral("AAPL"), bars);

    auto* run = child<QPushButton>(panel, "experimentRunButton");
    child<QComboBox>(panel, "experimentPriceField")->setCurrentIndex(1);
    run->click();
    QVERIFY(child<QLabel>(panel, "experimentStatusLabel")->text().contains(
        QStringLiteral("Adjusted close is missing")));
    QCOMPARE(child<QLabel>(panel, "experimentExecutionLabel")->text(),
             QStringLiteral("--"));

    child<QComboBox>(panel, "experimentPriceField")->setCurrentIndex(0);
    child<QDateEdit>(panel, "experimentStartDate")->setDate(QDate(2024, 1, 4));
    child<QDateEdit>(panel, "experimentEndDate")->setDate(QDate(2024, 1, 2));
    run->click();
    QCOMPARE(child<QLabel>(panel, "experimentStatusLabel")->text(),
             QStringLiteral("Invalid experiment time range"));
}

} // namespace

QTEST_MAIN(ExperimentPanelTests)

#include "experiment_panel_tests.moc"
