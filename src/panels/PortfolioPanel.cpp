#include "panels/PortfolioPanel.h"
#include "core/I18n.h"

#include <QDateTime>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include <cmath>
#include <unordered_map>

namespace fininsight::panels {
namespace {

QString money(double value)
{
    static const QLocale locale(QLocale::English, QLocale::UnitedStates);
    const QString amount = locale.toString(std::abs(value), 'f', 2);
    return value < 0.0
        ? QStringLiteral("-$%1").arg(amount)
        : QStringLiteral("$%1").arg(amount);
}

QString pnlText(double value, double returnRate)
{
    return QStringLiteral("%1 (%2%)")
        .arg(money(value))
        .arg(returnRate * 100.0, 0, 'f', 2);
}

QString pnlColor(double value)
{
    if (value > 0.0) return QStringLiteral("#d93025");
    if (value < 0.0) return QStringLiteral("#188038");
    return QStringLiteral("#5f6368");
}

} // namespace

PortfolioPanel::PortfolioPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* summary = new QWidget;
    auto* summaryGrid = new QGridLayout(summary);
    summaryGrid->setContentsMargins(8, 4, 8, 4);
    summaryGrid->setHorizontalSpacing(18);
    summaryGrid->setVerticalSpacing(0);
    summary->setStyleSheet(QStringLiteral(
        "background-color: #f8f9fa; border-bottom: 1px solid #e0e0e0;"));

    labelCash_ = new QLabel;
    labelValue_ = new QLabel;
    labelEquity_ = new QLabel;
    labelRealizedPnl_ = new QLabel;
    labelUnrealizedPnl_ = new QLabel;
    labelPnl_ = new QLabel;
    labelCash_->setObjectName(QStringLiteral("portfolioCashLabel"));
    labelValue_->setObjectName(QStringLiteral("portfolioHoldingsLabel"));
    labelEquity_->setObjectName(QStringLiteral("portfolioEquityLabel"));
    labelRealizedPnl_->setObjectName(QStringLiteral("portfolioRealizedPnlLabel"));
    labelUnrealizedPnl_->setObjectName(QStringLiteral("portfolioUnrealizedPnlLabel"));
    labelPnl_->setObjectName(QStringLiteral("portfolioTotalPnlLabel"));

    const auto addMetric = [this, summaryGrid](int row, int column, QLabel* value) {
        auto* captionLabel = new QLabel;
        captionLabel->setStyleSheet(QStringLiteral(
            "color: #5f6368; font-size: 11px; background: transparent; border: none;"));
        value->setStyleSheet(QStringLiteral(
            "color: #1e1e1e; font-weight: 600; background: transparent; border: none;"));
        metricCaptions_.append(captionLabel);
        summaryGrid->addWidget(captionLabel, row * 2, column);
        summaryGrid->addWidget(value, row * 2 + 1, column);
        summaryGrid->setColumnStretch(column, 1);
    };
    addMetric(0, 0, labelCash_);
    addMetric(0, 1, labelValue_);
    addMetric(0, 2, labelEquity_);
    addMetric(1, 0, labelRealizedPnl_);
    addMetric(1, 1, labelUnrealizedPnl_);
    addMetric(1, 2, labelPnl_);
    layout->addWidget(summary);

    auto* orderLayout = new QHBoxLayout;
    orderLayout->setContentsMargins(4, 0, 4, 0);

    labelSymbol_ = new QLabel;
    labelPrice_ = new QLabel;
    labelSymbol_->setObjectName(QStringLiteral("portfolioSymbolLabel"));
    labelPrice_->setObjectName(QStringLiteral("portfolioPriceLabel"));
    labelSymbol_->setStyleSheet(QStringLiteral("font-weight: 600; color: #1e1e1e;"));
    labelPrice_->setStyleSheet(QStringLiteral("color: #5f6368;"));
    orderLayout->addWidget(labelSymbol_);
    orderLayout->addWidget(labelPrice_);
    orderLayout->addStretch();

    spinQty_ = new QSpinBox;
    spinQty_->setObjectName(QStringLiteral("portfolioQuantitySpin"));
    spinQty_->setRange(1, 1000000);
    spinQty_->setValue(100);

    spinFee_ = new QDoubleSpinBox;
    spinFee_->setObjectName(QStringLiteral("portfolioFeeSpin"));
    spinFee_->setRange(0.0, 1000000.0);
    spinFee_->setDecimals(2);

    btnBuy_ = new QPushButton;
    btnBuy_->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    btnSell_ = new QPushButton;
    btnSell_->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    btnBuy_->setObjectName(QStringLiteral("portfolioBuyButton"));
    btnSell_->setObjectName(QStringLiteral("portfolioSellButton"));
    btnBuy_->setStyleSheet(QStringLiteral(
        "background-color: #d93025; color: white; font-weight: 600; border-radius: 4px;"));
    btnSell_->setStyleSheet(QStringLiteral(
        "background-color: #188038; color: white; font-weight: 600; border-radius: 4px;"));

    orderLayout->addWidget(spinQty_);
    orderLayout->addWidget(spinFee_);
    orderLayout->addWidget(btnBuy_);
    orderLayout->addWidget(btnSell_);
    layout->addLayout(orderLayout);

    labelPreview_ = new QLabel;
    labelPreview_->setObjectName(QStringLiteral("portfolioPreviewLabel"));
    labelPreview_->setStyleSheet(QStringLiteral("color: #1a73e8; font-size: 12px;"));
    labelPreview_->setFixedHeight(20);
    layout->addWidget(labelPreview_);

    labelStatus_ = new QLabel;
    labelStatus_->setObjectName(QStringLiteral("portfolioStatusLabel"));
    labelStatus_->setWordWrap(true);
    labelStatus_->setFixedHeight(22);
    layout->addWidget(labelStatus_);

    tradesTable_ = new QTableWidget(0, 7);
    tradesTable_->setObjectName(QStringLiteral("portfolioTradesTable"));
    tradesTable_->horizontalHeader()->setStretchLastSection(true);
    tradesTable_->verticalHeader()->setVisible(false);
    tradesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tradesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tradesTable_->setMinimumHeight(64);
    layout->addWidget(tradesTable_, 1);

    connect(btnBuy_, &QPushButton::clicked, this, &PortfolioPanel::onBuyClicked);
    connect(btnSell_, &QPushButton::clicked, this, &PortfolioPanel::onSellClicked);
    connect(tradesTable_, &QTableWidget::cellClicked,
            this, &PortfolioPanel::onTradeRowClicked);

    retranslateUi();
    refreshSummary();
    refreshTradeAvailability();
}

void PortfolioPanel::retranslateUi()
{
    const QStringList captions = {
        I18n::instance().t("Cash"), I18n::instance().t("Holdings"),
        I18n::instance().t("Total equity"), I18n::instance().t("Realized P&L"),
        I18n::instance().t("Unrealized P&L"), I18n::instance().t("Total P&L")};
    for (int i = 0; i < metricCaptions_.size() && i < captions.size(); ++i)
        metricCaptions_[i]->setText(captions[i]);

    spinQty_->setPrefix(I18n::instance().t("Qty: "));
    spinFee_->setPrefix(I18n::instance().t("Fee: $"));
    btnBuy_->setText(I18n::instance().t("Buy"));
    btnSell_->setText(I18n::instance().t("Sell"));

    tradesTable_->setHorizontalHeaderLabels({
        I18n::instance().t("Symbol"), I18n::instance().t("Side"),
        I18n::instance().t("Qty"), I18n::instance().t("Price"),
        I18n::instance().t("Fee"), I18n::instance().t("Realized P&L"),
        I18n::instance().t("Time")});

    if (labelSymbol_->text().isEmpty() ||
        labelSymbol_->text() == I18n::instance().t("No symbol selected"))
        labelSymbol_->setText(I18n::instance().t("No symbol selected"));
    if (currentSymbol_.isEmpty())
        labelPrice_->setText(I18n::instance().t("Waiting for quote"));
}

void PortfolioPanel::setCurrentSymbol(const QString& symbol)
{
    currentSymbol_ = symbol.trimmed().toUpper();
    labelSymbol_->setText(currentSymbol_.isEmpty()
        ? I18n::instance().t("No symbol selected") : currentSymbol_);
    showStatus({}, false);
    refreshTradeAvailability();
}

void PortfolioPanel::onQuoteUpdated(const datahub::QuoteData& quote)
{
    const QString symbol = quote.symbol.trimmed().toUpper();
    if (symbol.isEmpty() || !std::isfinite(quote.price) || quote.price <= 0.0) return;

    latestPrices_[symbol] = quote.price;
    refreshTradeAvailability();
    refreshSummary();
}

analysis::EvidenceSnapshot PortfolioPanel::evidenceSnapshot() const
{
    analysis::EvidenceSnapshot evidence;
    evidence.source = "portfolio-ledger";
    evidence.priceBasis = "latest-quote";
    evidence.trades = ledger_.trades();
    if (!evidence.trades.empty()) {
        evidence.startTimestampMs = evidence.trades.front().timestampMs;
        evidence.endTimestampMs = evidence.trades.back().timestampMs;
    }
    std::unordered_map<std::string, double> prices;
    for (auto it = latestPrices_.cbegin(); it != latestPrices_.cend(); ++it) {
        prices.emplace(it.key().toStdString(), it.value());
    }
    const auto valuation = ledger_.value(prices);
    if (valuation.valid) evidence.portfolio = valuation.snapshot;
    return evidence;
}

void PortfolioPanel::onBuyClicked()
{
    executeTrade(simulation::TradeSide::Buy);
}

void PortfolioPanel::onSellClicked()
{
    executeTrade(simulation::TradeSide::Sell);
}

void PortfolioPanel::executeTrade(simulation::TradeSide side)
{
    double price = 0.0;
    QString tradeDate;
    if (hasHistoricalPrice_) {
        price = historicalPrice_;
        tradeDate = historicalDate_;
    } else {
        const auto priceIt = latestPrices_.constFind(currentSymbol_);
        if (currentSymbol_.isEmpty() || priceIt == latestPrices_.cend()) {
            showStatus(I18n::instance().t("A valid quote is required before trading"), true);
            return;
        }
        price = priceIt.value();
        tradeDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    }

    simulation::TradeRequest request;
    request.side = side;
    request.symbol = currentSymbol_.toStdString();
    request.quantity = spinQty_->value();
    request.price = price;
    request.fee = spinFee_->value();
    request.timestampMs = QDateTime::fromString(tradeDate, "yyyy-MM-dd").toMSecsSinceEpoch();

    const auto result = ledger_.execute(request);
    if (!result.ok()) {
        showStatus(QString::fromStdString(result.message), true);
        return;
    }

    addTrade(result.trade);
    emit evidenceChanged();
    emit tradeExecuted(tradeDate, side == simulation::TradeSide::Buy);
    const QString action = side == simulation::TradeSide::Buy
        ? I18n::instance().t("Bought") : I18n::instance().t("Sold");
    showStatus(QStringLiteral("%1 %2 %3 at %4")
        .arg(action)
        .arg(result.trade.quantity)
        .arg(QString::fromStdString(result.trade.symbol))
        .arg(money(result.trade.price)), false);
    refreshSummary();
}

void PortfolioPanel::refreshSummary()
{
    std::unordered_map<std::string, double> prices;
    prices.reserve(static_cast<std::size_t>(latestPrices_.size()));
    for (auto it = latestPrices_.cbegin(); it != latestPrices_.cend(); ++it) {
        prices.emplace(it.key().toStdString(), it.value());
    }

    const auto valuation = ledger_.value(prices);
    if (!valuation.valid) {
        showStatus(QString::fromStdString(valuation.error), true);
        return;
    }

    const auto& snapshot = valuation.snapshot;
    labelCash_->setText(money(snapshot.cash));
    labelValue_->setText(money(snapshot.holdingsValue));
    labelEquity_->setText(money(snapshot.totalEquity));
    labelRealizedPnl_->setText(money(snapshot.realizedPnl));
    labelUnrealizedPnl_->setText(money(snapshot.unrealizedPnl));
    labelPnl_->setText(pnlText(snapshot.totalPnl, snapshot.returnRate));

    labelRealizedPnl_->setStyleSheet(
        QStringLiteral("color: %1;").arg(pnlColor(snapshot.realizedPnl)));
    labelUnrealizedPnl_->setStyleSheet(
        QStringLiteral("color: %1;").arg(pnlColor(snapshot.unrealizedPnl)));
    labelPnl_->setStyleSheet(
        QStringLiteral("color: %1; font-weight: 600;").arg(pnlColor(snapshot.totalPnl)));
}

void PortfolioPanel::refreshTradeAvailability()
{
    double price = 0.0;
    bool hasQuote = false;
    if (hasHistoricalPrice_) {
        price = historicalPrice_;
        hasQuote = !currentSymbol_.isEmpty() && historicalPrice_ > 0.0;
    } else {
        const auto priceIt = latestPrices_.constFind(currentSymbol_);
        hasQuote = !currentSymbol_.isEmpty()
            && priceIt != latestPrices_.cend()
            && std::isfinite(priceIt.value())
            && priceIt.value() > 0.0;
        if (hasQuote) price = priceIt.value();
    }
    btnBuy_->setEnabled(hasQuote);
    btnSell_->setEnabled(hasQuote);
    if (hasHistoricalPrice_) {
        labelPrice_->setText(QString("%1  %2").arg(historicalDate_, money(historicalPrice_)));
    } else if (hasQuote) {
        labelPrice_->setText(money(price));
    } else {
        labelPrice_->setText(I18n::instance().t("Waiting for quote"));
    }
}

void PortfolioPanel::setHistoricalBar(const datahub::KLineData& bar)
{
    historicalDate_ = bar.date;
    historicalPrice_ = bar.close;
    hasHistoricalPrice_ = true;
    refreshTradeAvailability();
    showStatus(QString("%1  %2").arg(bar.date, money(bar.close)), false);
}

void PortfolioPanel::clearHistoricalBar()
{
    hasHistoricalPrice_ = false;
    historicalDate_.clear();
    historicalPrice_ = 0.0;
    refreshTradeAvailability();
    showStatus({}, false);
}

void PortfolioPanel::previewAtPrice(double price)
{
    if (price <= 0.0) {
        labelPreview_->clear();
        return;
    }
    const auto it = ledger_.positions().find(currentSymbol_.toStdString());
    if (it == ledger_.positions().end() || it->second.quantity == 0) {
        labelPreview_->clear();
        return;
    }
    const double pnl = it->second.quantity * (price - it->second.averageCost());
    labelPreview_->setText(QString("%1  %2")
        .arg(money(price))
        .arg(pnl >= 0.0 ? QStringLiteral("+") + money(pnl) : money(pnl)));
}

void PortfolioPanel::addTrade(const simulation::Trade& trade)
{
    const int row = tradesTable_->rowCount();
    tradesTable_->insertRow(row);
    tradesTable_->setItem(row, 0,
        new QTableWidgetItem(QString::fromStdString(trade.symbol)));
    // 成交日期存到首列 UserRole，供点击成交记录时跳转到 K 线对应时点
    const QString tradeDate = QDateTime::fromMSecsSinceEpoch(trade.timestampMs)
        .toLocalTime().toString(QStringLiteral("yyyy-MM-dd"));
    tradesTable_->item(row, 0)->setData(Qt::UserRole, tradeDate);
    tradesTable_->setItem(row, 1, new QTableWidgetItem(
        trade.side == simulation::TradeSide::Buy ? I18n::instance().t("BUY")
                                                  : I18n::instance().t("SELL")));
    tradesTable_->setItem(row, 2,
        new QTableWidgetItem(QString::number(trade.quantity)));
    tradesTable_->setItem(row, 3, new QTableWidgetItem(money(trade.price)));
    tradesTable_->setItem(row, 4, new QTableWidgetItem(money(trade.fee)));
    tradesTable_->setItem(row, 5, new QTableWidgetItem(money(trade.realizedPnl)));
    tradesTable_->setItem(row, 6, new QTableWidgetItem(
        QDateTime::fromMSecsSinceEpoch(trade.timestampMs)
            .toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
    tradesTable_->scrollToBottom();
}

void PortfolioPanel::onTradeRowClicked(int row, int)
{
    if (row < 0 || row >= tradesTable_->rowCount()) return;
    auto* item = tradesTable_->item(row, 0);
    if (!item) return;
    const QString date = item->data(Qt::UserRole).toString();
    if (!date.isEmpty()) emit tradeSelected(date);
}

void PortfolioPanel::showStatus(const QString& message, bool isError)
{
    labelStatus_->setText(message);
    labelStatus_->setStyleSheet(isError
        ? QStringLiteral("color: #b3261e;")
        : QStringLiteral("color: #137333;"));
}

} // namespace fininsight::panels
