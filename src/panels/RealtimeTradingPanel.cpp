#include "panels/RealtimeTradingPanel.h"
#include "core/I18n.h"
#include "trading/OrderService.h"
#include "trading/PaperExecutionGateway.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
namespace fininsight::panels {
RealtimeTradingPanel::RealtimeTradingPanel(QWidget* parent) : QWidget(parent)
{
    gateway_ = std::make_unique<trading::PaperExecutionGateway>(100000.0);
    gateway_->onQuote("AAPL", 1.0, 1);
    orderService_ = std::make_unique<trading::OrderService>(*gateway_, trading::RiskEngine{});
    auto* root = new QVBoxLayout(this); root->setContentsMargins(8, 6, 8, 6);
    environment_ = new QLabel; environment_->setObjectName(QStringLiteral("tradingEnvironmentLabel"));
    environment_->setStyleSheet(QStringLiteral("font-weight:bold;color:#1565c0;")); root->addWidget(environment_);
    account_ = new QLabel; account_->setObjectName(QStringLiteral("tradingAccountLabel")); root->addWidget(account_);
    quoteStatus_ = new QLabel; quoteStatus_->setObjectName(QStringLiteral("tradingQuoteLabel")); root->addWidget(quoteStatus_);
    form_ = new QFormLayout; side_ = new QComboBox; side_->addItems({QStringLiteral("BUY"), QStringLiteral("SELL")});
    type_ = new QComboBox; type_->addItems({QStringLiteral("MARKET"), QStringLiteral("LIMIT")});
    quantity_ = new QSpinBox; quantity_->setRange(1, 1000); limitPrice_ = new QDoubleSpinBox; limitPrice_->setRange(0.01, 1000000); limitPrice_->setDecimals(2);
    side_->setObjectName(QStringLiteral("tradingSideCombo")); type_->setObjectName(QStringLiteral("tradingTypeCombo"));
    quantity_->setObjectName(QStringLiteral("tradingQuantitySpin")); limitPrice_->setObjectName(QStringLiteral("tradingLimitSpin"));
    form_->addRow(QString(), side_); form_->addRow(QString(), type_); form_->addRow(QString(), quantity_); form_->addRow(QString(), limitPrice_); root->addLayout(form_);
    connect(type_, &QComboBox::currentIndexChanged, this, [this](int index) { limitPrice_->setEnabled(index == 1); }); limitPrice_->setEnabled(false);
    submit_ = new QPushButton; submit_->setObjectName(QStringLiteral("tradingSubmitButton"));
    cancel_ = new QPushButton; cancel_->setObjectName(QStringLiteral("tradingCancelButton"));
    killSwitch_ = new QCheckBox; killSwitch_->setObjectName(QStringLiteral("tradingKillSwitch"));
    root->addWidget(submit_); root->addWidget(cancel_); root->addWidget(killSwitch_);
    message_ = new QLabel; message_->setObjectName(QStringLiteral("tradingMessageLabel")); message_->setWordWrap(true); root->addWidget(message_);
    orders_ = new QTableWidget(0, 6); orders_->setObjectName(QStringLiteral("tradingOrdersTable"));
    orders_->horizontalHeader()->setStretchLastSection(true); orders_->setEditTriggers(QAbstractItemView::NoEditTriggers); root->addWidget(orders_, 1);
    connect(submit_, &QPushButton::clicked, this, &RealtimeTradingPanel::submitOrder); connect(cancel_, &QPushButton::clicked, this, &RealtimeTradingPanel::cancelSelected);
    retranslateUi();
    refresh();
}

void RealtimeTradingPanel::retranslateUi()
{
    environment_->setText(I18n::instance().t("PAPER"));
    if (form_) {
        if (auto* l = qobject_cast<QLabel*>(form_->labelForField(side_))) l->setText(I18n::instance().t("Side"));
        if (auto* l = qobject_cast<QLabel*>(form_->labelForField(type_))) l->setText(I18n::instance().t("Type"));
        if (auto* l = qobject_cast<QLabel*>(form_->labelForField(quantity_))) l->setText(I18n::instance().t("Quantity"));
        if (auto* l = qobject_cast<QLabel*>(form_->labelForField(limitPrice_))) l->setText(I18n::instance().t("Limit"));
    }
    side_->setItemText(0, I18n::instance().t("BUY"));
    side_->setItemText(1, I18n::instance().t("SELL"));
    type_->setItemText(0, I18n::instance().t("MARKET"));
    type_->setItemText(1, I18n::instance().t("LIMIT"));
    submit_->setText(I18n::instance().t("Submit Paper Order"));
    cancel_->setText(I18n::instance().t("Cancel Selected"));
    killSwitch_->setText(I18n::instance().t("Kill switch"));
    orders_->setHorizontalHeaderLabels({I18n::instance().t("ID"), I18n::instance().t("Side"),
        I18n::instance().t("Symbol"), I18n::instance().t("Qty"),
        I18n::instance().t("Price"), I18n::instance().t("Status")});
}
RealtimeTradingPanel::~RealtimeTradingPanel() = default;
void RealtimeTradingPanel::setCurrentSymbol(const QString& symbol) { currentSymbol_ = symbol.trimmed().toUpper(); refresh(); }
void RealtimeTradingPanel::onQuoteUpdated(const datahub::QuoteData& quote)
{
    if (quote.symbol.trimmed().toUpper() != currentSymbol_ || !quote.isValid()) return; quote_ = quote;
    const auto changedOrders = gateway_->onQuote(quote.symbol.toStdString(), quote.price, quote.timestamp);
    for (const auto& order : changedOrders) emit orderChanged(order);
    limitPrice_->setValue(quote.price); refresh();
}
void RealtimeTradingPanel::submitOrder()
{
    trading::OrderRequest request; request.clientOrderId = QStringLiteral("paper-%1").arg(nextOrderId_++).toStdString();
    request.symbol = currentSymbol_.toStdString(); request.side = side_->currentIndex() == 0 ? trading::OrderSide::Buy : trading::OrderSide::Sell;
    request.type = type_->currentIndex() == 0 ? trading::OrderType::Market : trading::OrderType::Limit; request.quantity = quantity_->value();
    request.limitPrice = limitPrice_->value(); request.timestampMs = QDateTime::currentMSecsSinceEpoch();
    trading::RiskContext context; context.killSwitch = killSwitch_->isChecked();
    context.quoteAgeMs = quote_.timestamp > 0 ? qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - (quote_.timestamp < 100000000000LL ? quote_.timestamp * 1000 : quote_.timestamp)) : 999999999;
    const auto order = orderService_->submit(request, quote_.price, context);
    emit orderChanged(order);
    message_->setText(QString::fromStdString(order.rejectionReason.empty() ? trading::orderStatusName(order.status) : order.rejectionReason)); refresh();
}
void RealtimeTradingPanel::cancelSelected()
{
    const int row = orders_->currentRow(); if (row < 0) return; const auto id = orders_->item(row, 0)->text().toStdString();
    const bool cancelled = orderService_->cancel(id);
    if (cancelled) {
        if (const auto order = gateway_->find(id)) emit orderChanged(*order);
    }
    message_->setText(cancelled ? I18n::instance().t("Order cancelled") : I18n::instance().t("Order cannot be cancelled")); refresh();
}
void RealtimeTradingPanel::refresh()
{
    const auto account = gateway_->account(); account_->setText(I18n::instance().t("Cash: $%1 | Equity: $%2 | %3 position: %4").arg(account.cash, 0, 'f', 2).arg(account.equity, 0, 'f', 2).arg(currentSymbol_.isEmpty() ? QStringLiteral("-") : currentSymbol_).arg(gateway_->position(currentSymbol_.toStdString())));
    quoteStatus_->setText(quote_.isValid() ? QStringLiteral("%1 @ $%2").arg(quote_.symbol).arg(quote_.price, 0, 'f', 2) : I18n::instance().t("No current quote"));
    submit_->setEnabled(!currentSymbol_.isEmpty() && quote_.isValid()); orders_->setRowCount(0);
    for (const auto& order : gateway_->orders()) { const int row = orders_->rowCount(); orders_->insertRow(row);
        orders_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(order.request.clientOrderId))); orders_->setItem(row, 1, new QTableWidgetItem(order.request.side == trading::OrderSide::Buy ? "BUY" : "SELL"));
        orders_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(order.request.symbol))); orders_->setItem(row, 3, new QTableWidgetItem(QString::number(order.request.quantity)));
        orders_->setItem(row, 4, new QTableWidgetItem(QString::number(order.averageFillPrice > 0 ? order.averageFillPrice : order.request.limitPrice, 'f', 2))); orders_->setItem(row, 5, new QTableWidgetItem(QString::fromLatin1(trading::orderStatusName(order.status)))); }
}
} // namespace fininsight::panels
