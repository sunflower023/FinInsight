#include "panels/PortfolioPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDateTime>
#include <QDebug>

namespace fininsight::panels {

PortfolioPanel::PortfolioPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    // —— 账户摘要 ——
    auto* summaryGroup = new QGroupBox("Account Summary");
    auto* summaryForm = new QFormLayout(summaryGroup);

    labelCash_  = new QLabel("$100,000.00");
    labelValue_ = new QLabel("$0.00");
    labelPnl_   = new QLabel("$0.00 (0.00%)");
    labelPnl_->setStyleSheet("color: gray;");

    summaryForm->addRow("Cash:",       labelCash_);
    summaryForm->addRow("Holdings:",   labelValue_);
    summaryForm->addRow("Total P&L:",  labelPnl_);
    layout->addWidget(summaryGroup);

    // —— 下单区 ——
    auto* tradeGroup = new QGroupBox("Quick Trade");
    auto* tradeLayout = new QHBoxLayout(tradeGroup);

    spinQty_ = new QSpinBox();
    spinQty_->setRange(1, 10000);
    spinQty_->setValue(100);
    spinQty_->setPrefix("Qty: ");

    auto* btnBuy  = new QPushButton("Buy");
    auto* btnSell = new QPushButton("Sell");
    btnBuy->setStyleSheet("background-color: #d93025; color: white; font-weight: 600; border-radius: 4px;");
    btnSell->setStyleSheet("background-color: #188038; color: white; font-weight: 600; border-radius: 4px;");

    tradeLayout->addWidget(spinQty_);
    tradeLayout->addWidget(btnBuy);
    tradeLayout->addWidget(btnSell);
    layout->addWidget(tradeGroup);

    // —— 交易记录表 ——
    auto* tradesGroup = new QGroupBox("Trade History");
    auto* tradesLayout = new QVBoxLayout(tradesGroup);

    tradesTable_ = new QTableWidget(0, 5);
    tradesTable_->setHorizontalHeaderLabels({"Symbol", "Type", "Qty", "Price", "Time"});
    tradesTable_->horizontalHeader()->setStretchLastSection(true);
    tradesTable_->verticalHeader()->setVisible(false);
    tradesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tradesLayout->addWidget(tradesTable_);
    layout->addWidget(tradesGroup);

    connect(btnBuy,  &QPushButton::clicked, this, &PortfolioPanel::onBuyClicked);
    connect(btnSell, &QPushButton::clicked, this, &PortfolioPanel::onSellClicked);
}

void PortfolioPanel::onBuyClicked() {
    // 获取当前关注的股票（通过父级 MainWindow 通信，简化处理）
    int qty = spinQty_->value();
    if (qty <= 0) return;
    // 这里价格从最近一次 QuoteData 获取，后面通过 DataHub 联动
}

void PortfolioPanel::onSellClicked() {
    int qty = spinQty_->value();
    if (qty <= 0) return;
}

void PortfolioPanel::refreshSummary() {
    double holdingsValue = 0.0, totalCost = 0.0;
    for (auto& pos : positions_) {
        totalCost += pos.totalCost;
        // holdingsValue 需要当前价格 → 后面从 QuoteData 补
    }
    // 暂时展示现金
    labelCash_->setText(QString("$%1").arg(cash_, 0, 'f', 2));
}

void PortfolioPanel::onQuoteUpdated(const datahub::QuoteData& quote) {
    // 后续完善：收到行情后更新浮动盈亏
    Q_UNUSED(quote);
}

void PortfolioPanel::addTrade(const Trade& trade) {
    trades_.append(trade);
    int row = tradesTable_->rowCount();
    tradesTable_->insertRow(row);
    tradesTable_->setItem(row, 0, new QTableWidgetItem(trade.symbol));
    tradesTable_->setItem(row, 1, new QTableWidgetItem(trade.type));
    tradesTable_->setItem(row, 2, new QTableWidgetItem(QString::number(trade.quantity)));
    tradesTable_->setItem(row, 3, new QTableWidgetItem(
        QString("$%1").arg(trade.price, 0, 'f', 2)));
    tradesTable_->setItem(row, 4, new QTableWidgetItem(trade.time));
}

} // namespace fininsight::panels
