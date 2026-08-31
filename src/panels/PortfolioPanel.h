#pragma once

#include "datahub/QuoteData.h"
#include "simulation/Ledger.h"
#include "analysis/BehaviorAnalyzer.h"

#include <QHash>
#include <QVector>
#include <QWidget>

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace fininsight::panels {

/**
 * @brief Simulated portfolio UI backed by the simulation domain ledger.
 */
class PortfolioPanel : public QWidget {
    Q_OBJECT

public:
    explicit PortfolioPanel(QWidget* parent = nullptr);

    /// 语言切换时刷新界面文本
    void retranslateUi();

    void setCurrentSymbol(const QString& symbol);
    void onQuoteUpdated(const datahub::QuoteData& quote);

    /// 选中 K 线某个时点 → 切换到历史价模式（用该时点收盘价买卖）
    void setHistoricalBar(const datahub::KLineData& bar);
    /// 清除历史价模式，回到实时价
    void clearHistoricalBar();
    /// 悬停到某价位时预览当前持仓的浮盈亏
    void previewAtPrice(double price);

    analysis::EvidenceSnapshot evidenceSnapshot() const;

signals:
    void evidenceChanged();
    /// 成交后发出（date=成交日期，isBuy=方向），供 K 线画买卖标记
    void tradeExecuted(const QString& date, bool isBuy);
    /// 点击成交记录某行时发出，供 K 线跳转到对应时点
    void tradeSelected(const QString& date);

private slots:
    void onBuyClicked();
    void onSellClicked();

private:
    void executeTrade(simulation::TradeSide side);
    void refreshSummary();
    void refreshTradeAvailability();
    void addTrade(const simulation::Trade& trade);
    void showStatus(const QString& message, bool isError);
    void onTradeRowClicked(int row, int column);

    simulation::Ledger ledger_{100000.0};
    QHash<QString, double> latestPrices_;
    QString currentSymbol_;

    QLabel* labelSymbol_ = nullptr;
    QLabel* labelPrice_ = nullptr;
    QLabel* labelPreview_ = nullptr;
    QLabel* labelCash_ = nullptr;
    QLabel* labelValue_ = nullptr;
    QLabel* labelEquity_ = nullptr;
    QLabel* labelRealizedPnl_ = nullptr;
    QLabel* labelUnrealizedPnl_ = nullptr;
    QLabel* labelPnl_ = nullptr;
    QLabel* labelStatus_ = nullptr;
    QSpinBox* spinQty_ = nullptr;
    QDoubleSpinBox* spinFee_ = nullptr;
    QPushButton* btnBuy_ = nullptr;
    QPushButton* btnSell_ = nullptr;
    QTableWidget* tradesTable_ = nullptr;

    // 顶部 6 个指标标题（Cash/Holdings/... 按顺序，供语言切换刷新）
    QVector<QLabel*> metricCaptions_;

    // —— 历史价模式（点击 K 线选时点）——
    double historicalPrice_ = 0.0;
    QString historicalDate_;
    bool hasHistoricalPrice_ = false;
};

} // namespace fininsight::panels
