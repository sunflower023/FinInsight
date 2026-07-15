#pragma once

#include <QLineEdit>
#include <QTimer>

namespace fininsight::panels {

/**
 * @brief 股票搜索框 — 输入代码回车触发查询
 *
 * 特性：实时输入防抖 300ms，回车立即查询
 */
class StockSearchBar : public QLineEdit {
    Q_OBJECT

public:
    explicit StockSearchBar(QWidget* parent = nullptr);

signals:
    /// 用户触发查询（回车或防抖超时）
    void searchRequested(const QString& symbol);

private slots:
    void onTextChanged(const QString& text);
    void onDebounceTimeout();

private:
    void onReturnPressed();
    QTimer* debounceTimer_;
    QString lastSymbol_;
};

} // namespace fininsight::panels
