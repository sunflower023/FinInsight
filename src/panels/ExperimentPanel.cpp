#include "panels/ExperimentPanel.h"
#include "core/I18n.h"

#include "datahub/HistoricalPriceAdapter.h"
#include "simulation/InvestmentExperiment.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include <cmath>

namespace fininsight::panels {
namespace {

QString money(double value)
{
    static const QLocale locale(QLocale::English, QLocale::UnitedStates);
    const QString amount = locale.toString(std::abs(value), 'f', 2);
    return value < 0.0 ? QStringLiteral("-$%1").arg(amount)
                       : QStringLiteral("$%1").arg(amount);
}

QString percentage(double ratio)
{
    return QStringLiteral("%1%").arg(ratio * 100.0, 0, 'f', 2);
}

QString resultColor(double value)
{
    if (value > 0.0) return QStringLiteral("#d93025");
    if (value < 0.0) return QStringLiteral("#188038");
    return QStringLiteral("#5f6368");
}

} // namespace

ExperimentPanel::ExperimentPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(5);

    labelData_ = new QLabel;
    labelData_->setObjectName(QStringLiteral("experimentDataLabel"));
    labelData_->setStyleSheet(QStringLiteral("font-weight: 600; color: #1e1e1e;"));
    layout->addWidget(labelData_);

    auto* inputs = new QGridLayout;
    inputs->setContentsMargins(0, 0, 0, 0);
    inputs->setHorizontalSpacing(8);
    inputs->setVerticalSpacing(1);

    startDate_ = new QDateEdit;
    endDate_ = new QDateEdit;
    initialCash_ = new QDoubleSpinBox;
    fee_ = new QDoubleSpinBox;
    priceField_ = new QComboBox;
    runButton_ = new QPushButton;
    runButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    startDate_->setObjectName(QStringLiteral("experimentStartDate"));
    endDate_->setObjectName(QStringLiteral("experimentEndDate"));
    initialCash_->setObjectName(QStringLiteral("experimentInitialCash"));
    fee_->setObjectName(QStringLiteral("experimentFee"));
    priceField_->setObjectName(QStringLiteral("experimentPriceField"));
    runButton_->setObjectName(QStringLiteral("experimentRunButton"));

    for (auto* dateEdit : {startDate_, endDate_}) {
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    }
    initialCash_->setRange(0.01, 1000000000000.0);
    initialCash_->setDecimals(2);
    initialCash_->setValue(100000.0);
    initialCash_->setPrefix(QStringLiteral("$"));
    initialCash_->setGroupSeparatorShown(true);
    fee_->setRange(0.0, 1000000.0);
    fee_->setDecimals(2);
    fee_->setPrefix(QStringLiteral("$"));
    fee_->setGroupSeparatorShown(true);
    priceField_->addItem(I18n::instance().t("Close"),
        static_cast<int>(simulation::HistoricalPriceField::Close));
    priceField_->addItem(I18n::instance().t("Adjusted close"),
        static_cast<int>(simulation::HistoricalPriceField::AdjustedClose));
    runButton_->setEnabled(false);

    const QList<QWidget*> controls = {
        startDate_, endDate_, initialCash_, fee_, priceField_, runButton_};
    for (int column = 0; column < controls.size(); ++column) {
        auto* caption = new QLabel;
        caption->setStyleSheet(QStringLiteral("color: #5f6368; font-size: 11px;"));
        inputCaptions_.append(caption);
        inputs->addWidget(caption, 0, column);
        inputs->addWidget(controls[column], 1, column);
        inputs->setColumnStretch(column, column == 2 || column == 4 ? 2 : 1);
    }
    layout->addLayout(inputs);

    labelStatus_ = new QLabel;
    labelStatus_->setObjectName(QStringLiteral("experimentStatusLabel"));
    labelStatus_->setFixedHeight(22);
    layout->addWidget(labelStatus_);

    auto* results = new QGridLayout;
    results->setContentsMargins(8, 3, 8, 3);
    results->setHorizontalSpacing(18);
    results->setVerticalSpacing(0);

    labelExecution_ = new QLabel;
    labelEnding_ = new QLabel;
    labelQuantity_ = new QLabel;
    labelEndingCash_ = new QLabel;
    labelMarketValue_ = new QLabel;
    labelEquity_ = new QLabel;
    labelPnl_ = new QLabel;
    labelReturn_ = new QLabel;
    labelDrawdown_ = new QLabel;
    labelExecution_->setObjectName(QStringLiteral("experimentExecutionLabel"));
    labelEnding_->setObjectName(QStringLiteral("experimentEndingLabel"));
    labelQuantity_->setObjectName(QStringLiteral("experimentQuantityLabel"));
    labelEndingCash_->setObjectName(QStringLiteral("experimentEndingCashLabel"));
    labelMarketValue_->setObjectName(QStringLiteral("experimentMarketValueLabel"));
    labelEquity_->setObjectName(QStringLiteral("experimentEquityLabel"));
    labelPnl_->setObjectName(QStringLiteral("experimentPnlLabel"));
    labelReturn_->setObjectName(QStringLiteral("experimentReturnLabel"));
    labelDrawdown_->setObjectName(QStringLiteral("experimentDrawdownLabel"));

    const QList<QLabel*> resultValues = {
        labelExecution_, labelEnding_, labelQuantity_, labelEndingCash_, labelMarketValue_,
        labelEquity_, labelPnl_, labelReturn_, labelDrawdown_};
    for (int index = 0; index < resultValues.size(); ++index) {
        const int metricRow = index / 3;
        const int column = index % 3;
        auto* caption = new QLabel;
        caption->setStyleSheet(QStringLiteral("color: #5f6368; font-size: 11px;"));
        resultCaptions_.append(caption);
        auto* cell = new QWidget;
        auto* cellLayout = new QHBoxLayout(cell);
        cellLayout->setContentsMargins(0, 1, 0, 1);
        cellLayout->setSpacing(5);
        cellLayout->addWidget(caption);
        cellLayout->addWidget(resultValues[index]);
        cellLayout->addStretch();
        results->addWidget(cell, metricRow, column);
        results->setColumnStretch(column, 1);
    }
    layout->addLayout(results, 1);

    connect(runButton_, &QPushButton::clicked,
            this, &ExperimentPanel::runExperiment);
    retranslateUi();
    resetResults();
}

void ExperimentPanel::retranslateUi()
{
    labelData_->setText(currentSymbol_.isEmpty()
        ? I18n::instance().t("Select a symbol and wait for daily bars")
        : I18n::instance().t("%1 | Waiting for daily bars").arg(currentSymbol_));

    const QStringList inputCaptions = {
        I18n::instance().t("From"), I18n::instance().t("To"),
        I18n::instance().t("Initial cash"), I18n::instance().t("Buy fee"),
        I18n::instance().t("Price"), I18n::instance().t("Action")};
    for (int i = 0; i < inputCaptions_.size() && i < inputCaptions.size(); ++i)
        inputCaptions_[i]->setText(inputCaptions[i]);

    const QStringList resultCaptions = {
        I18n::instance().t("Execution"), I18n::instance().t("Ending"),
        I18n::instance().t("Quantity"), I18n::instance().t("Remaining cash"),
        I18n::instance().t("Market value"), I18n::instance().t("Ending equity"),
        I18n::instance().t("Total P&L"), I18n::instance().t("Return"),
        I18n::instance().t("Max drawdown")};
    for (int i = 0; i < resultCaptions_.size() && i < resultCaptions.size(); ++i)
        resultCaptions_[i]->setText(resultCaptions[i]);

    runButton_->setText(I18n::instance().t("Run"));
    priceField_->setItemText(0, I18n::instance().t("Close"));
    priceField_->setItemText(1, I18n::instance().t("Adjusted close"));
}

void ExperimentPanel::setCurrentSymbol(const QString& symbol)
{
    currentSymbol_ = symbol.trimmed().toUpper();
    bars_.clear();
    runButton_->setEnabled(false);
    labelData_->setText(currentSymbol_.isEmpty()
        ? I18n::instance().t("Select a symbol and wait for daily bars")
        : I18n::instance().t("%1 | Waiting for daily bars").arg(currentSymbol_));
    showStatus({}, false);
    resetResults();
}

void ExperimentPanel::setHistoricalData(
    const QString& symbol, const QVector<datahub::KLineData>& bars)
{
    const QString normalized = symbol.trimmed().toUpper();
    if (normalized.isEmpty() || normalized != currentSymbol_) return;

    const auto validation = datahub::toHistoricalPriceSeries(
        normalized, bars, simulation::HistoricalPriceField::Close);
    if (!validation.ok()) {
        bars_.clear();
        runButton_->setEnabled(false);
        labelData_->setText(I18n::instance().t("%1 | Historical data unavailable").arg(normalized));
        showStatus(QString::fromStdString(validation.message), true);
        resetResults();
        return;
    }

    bars_ = bars;
    const QDate firstDate = QDate::fromString(bars_.first().date, Qt::ISODate);
    const QDate lastDate = QDate::fromString(bars_.last().date, Qt::ISODate);
    startDate_->setDateRange(firstDate, lastDate);
    endDate_->setDateRange(firstDate, lastDate);
    startDate_->setDate(firstDate);
    endDate_->setDate(lastDate);
    labelData_->setText(I18n::instance().t("%1 | %2 daily bars | %3 to %4")
        .arg(normalized)
        .arg(bars_.size())
        .arg(firstDate.toString(Qt::ISODate), lastDate.toString(Qt::ISODate)));
    runButton_->setEnabled(true);
    showStatus({}, false);
    resetResults();
}

void ExperimentPanel::runExperiment()
{
    if (currentSymbol_.isEmpty() || bars_.isEmpty()) {
        showStatus(I18n::instance().t("Historical daily bars are required"), true);
        return;
    }

    const auto priceField = static_cast<simulation::HistoricalPriceField>(
        priceField_->currentData().toInt());
    const auto series = datahub::toHistoricalPriceSeries(
        currentSymbol_, bars_, priceField);
    if (!series.ok()) {
        resetResults();
        showStatus(QString::fromStdString(series.message), true);
        return;
    }

    simulation::InvestmentExperimentRequest request;
    request.symbol = currentSymbol_.toStdString();
    request.initialCash = initialCash_->value();
    request.startTimestampMs = QDateTime(
        startDate_->date(), QTime(0, 0), Qt::UTC).toMSecsSinceEpoch();
    request.endTimestampMs = QDateTime(
        endDate_->date(), QTime(0, 0), Qt::UTC).toMSecsSinceEpoch();
    request.buyFee = fee_->value();

    const auto result = simulation::runBuyAndHoldExperiment(request, series.prices);
    if (!result.ok()) {
        resetResults();
        showStatus(QString::fromStdString(result.message), true);
        return;
    }

    lastResult_ = result;
    hasResult_ = true;
    lastInitialCash_ = request.initialCash;
    lastBuyFee_ = request.buyFee;
    lastStartTimestampMs_ = request.startTimestampMs;
    lastEndTimestampMs_ = request.endTimestampMs;

    const QString executionDate = QDateTime::fromMSecsSinceEpoch(
        result.executedTimestampMs, Qt::UTC).date().toString(Qt::ISODate);
    const QString endingDate = QDateTime::fromMSecsSinceEpoch(
        result.endingTimestampMs, Qt::UTC).date().toString(Qt::ISODate);
    setResultValue(labelExecution_,
        I18n::instance().t("%1 at %2").arg(executionDate, money(result.executedPrice)));
    setResultValue(labelEnding_,
        I18n::instance().t("%1 at %2").arg(endingDate, money(result.endingPrice)));
    setResultValue(labelQuantity_, QString::number(result.quantity));
    setResultValue(labelEndingCash_, money(result.endingCash));
    setResultValue(labelMarketValue_, money(result.endingMarketValue));
    setResultValue(labelEquity_, money(result.endingEquity));
    setResultValue(labelPnl_, money(result.totalPnl), resultColor(result.totalPnl));
    setResultValue(labelReturn_, percentage(result.returnRate), resultColor(result.returnRate));
    setResultValue(labelDrawdown_, percentage(result.maxDrawdown),
                   result.maxDrawdown > 0.0 ? QStringLiteral("#b3261e")
                                            : QStringLiteral("#5f6368"));
    showStatus(I18n::instance().t("Experiment completed using %1")
        .arg(priceField_->currentText()), false);
    emit evidenceChanged();
}

analysis::EvidenceSnapshot ExperimentPanel::evidenceSnapshot() const
{
    analysis::EvidenceSnapshot evidence;
    if (!hasResult_) return evidence;
    evidence.source = "historical-experiment";
    evidence.priceBasis = priceField_->currentText().toStdString();
    evidence.startTimestampMs = lastStartTimestampMs_;
    evidence.endTimestampMs = lastEndTimestampMs_;
    evidence.maxDrawdown = lastResult_.maxDrawdown;
    evidence.portfolio.initialCash = lastInitialCash_;
    evidence.portfolio.cash = lastResult_.endingCash;
    evidence.portfolio.holdingsValue = lastResult_.endingMarketValue;
    evidence.portfolio.totalEquity = lastResult_.endingEquity;
    evidence.portfolio.totalPnl = lastResult_.totalPnl;
    evidence.portfolio.returnRate = lastResult_.returnRate;
    evidence.trades.push_back({1, simulation::TradeSide::Buy, lastResult_.symbol,
                               lastResult_.quantity, lastResult_.executedPrice,
                               lastBuyFee_, lastResult_.executedTimestampMs, 0.0});
    return evidence;
}

void ExperimentPanel::resetResults()
{
    for (auto* label : {labelExecution_, labelEnding_, labelQuantity_,
                        labelEndingCash_, labelMarketValue_, labelEquity_,
                        labelPnl_, labelReturn_, labelDrawdown_}) {
        setResultValue(label, QStringLiteral("--"));
    }
}

void ExperimentPanel::showStatus(const QString& message, bool isError)
{
    labelStatus_->setText(message);
    labelStatus_->setVisible(!message.isEmpty());
    labelStatus_->setStyleSheet(isError
        ? QStringLiteral("color: #b3261e;")
        : QStringLiteral("color: #137333;"));
}

void ExperimentPanel::setResultValue(
    QLabel* label, const QString& value, const QString& color)
{
    label->setText(value);
    label->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;").arg(color));
}

} // namespace fininsight::panels
