#pragma once

#include "datahub/QuoteData.h"

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>

namespace fininsight::panels {

/// 一笔交易记录
struct Trade {
    QString symbol;
    QString type;       // "BUY" / "SELL"
    int     quantity;
    double  price;
    QString time;
};

/**
 * @brief 模拟投资组合面板 — 记录买卖，实时计算盈亏
 */
class PortfolioPanel : public QWidget {
    Q_OBJECT

public:
    explicit PortfolioPanel(QWidget* parent = nullptr);

    /// 接收报价更新盈亏计算
    void onQuoteUpdated(const datahub::QuoteData& quote);

private slots:
    void onBuyClicked();
    void onSellClicked();

private:
    void refreshSummary();
    void addTrade(const Trade& trade);

    // 当前持仓: symbol → {totalQty, totalCost}
    struct Position {
        int totalQty = 0;
        double totalCost = 0.0;
    };
    QHash<QString, Position> positions_;
    QVector<Trade> trades_;

    // UI
    QLabel*      labelCash_;
    QLabel*      labelValue_;
    QLabel*      labelPnl_;
    QSpinBox*    spinQty_;
    QTableWidget* tradesTable_;

    double cash_ = 100000.0;  // 初始资金 10 万
};

} // namespace fininsight::panels
