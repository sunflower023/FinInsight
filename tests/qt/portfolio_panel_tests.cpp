#include "panels/PortfolioPanel.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTest>

namespace {

using fininsight::datahub::QuoteData;
using fininsight::panels::PortfolioPanel;

template <typename T>
T* child(PortfolioPanel& panel, const char* objectName)
{
    auto* result = panel.findChild<T*>(QString::fromLatin1(objectName));
    Q_ASSERT(result);
    return result;
}

QuoteData quote(const QString& symbol, double price)
{
    QuoteData result;
    result.symbol = symbol;
    result.price = price;
    result.timestamp = 1;
    return result;
}

class PortfolioPanelTests final : public QObject {
    Q_OBJECT

private slots:
    void tradeControlsRequireCurrentQuote();
    void buyRevalueSellAndRejectInvalidOrder();
};

void PortfolioPanelTests::tradeControlsRequireCurrentQuote()
{
    PortfolioPanel panel;
    auto* buy = child<QPushButton>(panel, "portfolioBuyButton");
    auto* sell = child<QPushButton>(panel, "portfolioSellButton");
    auto* symbol = child<QLabel>(panel, "portfolioSymbolLabel");
    auto* price = child<QLabel>(panel, "portfolioPriceLabel");

    QCOMPARE(child<QLabel>(panel, "portfolioCashLabel")->text(),
             QStringLiteral("$100,000.00"));
    QVERIFY(!buy->isEnabled());
    QVERIFY(!sell->isEnabled());

    panel.setCurrentSymbol(QStringLiteral(" aapl "));
    QCOMPARE(symbol->text(), QStringLiteral("AAPL"));
    QVERIFY(!buy->isEnabled());
    QCOMPARE(price->text(), QStringLiteral("Waiting for quote"));

    panel.onQuoteUpdated(quote(QStringLiteral("MSFT"), 200.0));
    QVERIFY(!buy->isEnabled());
    panel.onQuoteUpdated(quote(QStringLiteral("AAPL"), 100.0));
    QVERIFY(buy->isEnabled());
    QVERIFY(sell->isEnabled());
    QCOMPARE(price->text(), QStringLiteral("$100.00"));
}

void PortfolioPanelTests::buyRevalueSellAndRejectInvalidOrder()
{
    PortfolioPanel panel;
    panel.setCurrentSymbol(QStringLiteral("AAPL"));
    panel.onQuoteUpdated(quote(QStringLiteral("AAPL"), 100.0));

    auto* quantity = child<QSpinBox>(panel, "portfolioQuantitySpin");
    auto* fee = child<QDoubleSpinBox>(panel, "portfolioFeeSpin");
    auto* buy = child<QPushButton>(panel, "portfolioBuyButton");
    auto* sell = child<QPushButton>(panel, "portfolioSellButton");
    auto* table = child<QTableWidget>(panel, "portfolioTradesTable");
    auto* status = child<QLabel>(panel, "portfolioStatusLabel");

    quantity->setValue(1000000);
    buy->click();
    QCOMPARE(table->rowCount(), 0);
    QCOMPARE(status->text(), QStringLiteral("Insufficient cash"));

    quantity->setValue(10);
    fee->setValue(5.0);
    buy->click();

    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("AAPL"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("BUY"));
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("10"));
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("$100.00"));
    QCOMPARE(table->item(0, 4)->text(), QStringLiteral("$5.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioCashLabel")->text(),
             QStringLiteral("$98,995.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioHoldingsLabel")->text(),
             QStringLiteral("$1,000.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioEquityLabel")->text(),
             QStringLiteral("$99,995.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioTotalPnlLabel")->text(),
             QStringLiteral("-$5.00 (-0.01%)"));

    panel.onQuoteUpdated(quote(QStringLiteral("AAPL"), 110.0));
    QCOMPARE(child<QLabel>(panel, "portfolioHoldingsLabel")->text(),
             QStringLiteral("$1,100.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioUnrealizedPnlLabel")->text(),
             QStringLiteral("$95.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioTotalPnlLabel")->text(),
             QStringLiteral("$95.00 (0.10%)"));

    quantity->setValue(4);
    fee->setValue(2.0);
    sell->click();
    QCOMPARE(table->rowCount(), 2);
    QCOMPARE(table->item(1, 1)->text(), QStringLiteral("SELL"));
    QCOMPARE(table->item(1, 5)->text(), QStringLiteral("$36.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioCashLabel")->text(),
             QStringLiteral("$99,433.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioRealizedPnlLabel")->text(),
             QStringLiteral("$36.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioUnrealizedPnlLabel")->text(),
             QStringLiteral("$57.00"));
    QCOMPARE(child<QLabel>(panel, "portfolioTotalPnlLabel")->text(),
             QStringLiteral("$93.00 (0.09%)"));

    quantity->setValue(100);
    sell->click();
    QCOMPARE(table->rowCount(), 2);
    QCOMPARE(status->text(), QStringLiteral("Insufficient position"));
    QCOMPARE(child<QLabel>(panel, "portfolioCashLabel")->text(),
             QStringLiteral("$99,433.00"));
}

} // namespace

QTEST_MAIN(PortfolioPanelTests)

#include "portfolio_panel_tests.moc"
