#pragma once

#include "datahub/QuoteData.h"

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace fininsight::panels {

/**
 * @brief 自选股列表面板 — 增删查，双击跳转 K 线
 */
class StockListPanel : public QWidget {
    Q_OBJECT

public:
    explicit StockListPanel(QWidget* parent = nullptr);

    /// 外部调：添加一只股票到列表
    void addStock(const QString& symbol, const QString& name);

    /// 外部调：更新价格信息
    void updatePrice(const QString& symbol, double price, double changePercent);

signals:
    /// 用户双击某只股票 → 触发 K 线图切换
    void stockSelected(const QString& symbol);

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onRemoveClicked();

private:
    QListWidget* listWidget_;
    QPushButton* btnRemove_;
};

} // namespace fininsight::panels
