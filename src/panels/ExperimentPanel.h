#pragma once

#include "datahub/QuoteData.h"
#include "analysis/BehaviorAnalyzer.h"
#include "simulation/InvestmentExperiment.h"

#include <QVector>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace fininsight::panels {

/**
 * @brief Historical buy-and-hold experiment inputs and result presentation.
 */
class ExperimentPanel : public QWidget {
    Q_OBJECT

public:
    explicit ExperimentPanel(QWidget* parent = nullptr);

    /// 语言切换时刷新界面文本
    void retranslateUi();

    void setCurrentSymbol(const QString& symbol);
    void setHistoricalData(const QString& symbol,
                           const QVector<datahub::KLineData>& bars);
    analysis::EvidenceSnapshot evidenceSnapshot() const;

signals:
    void evidenceChanged();

private slots:
    void runExperiment();

private:
    void resetResults();
    void showStatus(const QString& message, bool isError);
    void setResultValue(QLabel* label, const QString& value,
                        const QString& color = QStringLiteral("#1e1e1e"));

    QString currentSymbol_;
    QVector<datahub::KLineData> bars_;

    QLabel* labelData_ = nullptr;
    QDateEdit* startDate_ = nullptr;
    QDateEdit* endDate_ = nullptr;
    QDoubleSpinBox* initialCash_ = nullptr;
    QDoubleSpinBox* fee_ = nullptr;
    QComboBox* priceField_ = nullptr;
    QPushButton* runButton_ = nullptr;
    QLabel* labelStatus_ = nullptr;
    QLabel* labelExecution_ = nullptr;
    QLabel* labelEnding_ = nullptr;
    QLabel* labelQuantity_ = nullptr;
    QLabel* labelEndingCash_ = nullptr;
    QLabel* labelMarketValue_ = nullptr;
    QLabel* labelEquity_ = nullptr;
    QLabel* labelPnl_ = nullptr;
    QLabel* labelReturn_ = nullptr;
    QLabel* labelDrawdown_ = nullptr;
    // 输入区 / 结果区的 caption 标签（按顺序，供语言切换刷新）
    QVector<QLabel*> inputCaptions_;
    QVector<QLabel*> resultCaptions_;
    simulation::InvestmentExperimentResult lastResult_;
    bool hasResult_ = false;
    double lastInitialCash_ = 0.0;
    double lastBuyFee_ = 0.0;
    qint64 lastStartTimestampMs_ = 0;
    qint64 lastEndTimestampMs_ = 0;
};

} // namespace fininsight::panels
