#pragma once

#include "datahub/QuoteData.h"

#include <QWidget>
#include <QTableWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QGroupBox;

namespace fininsight::panels {

/**
 * @brief 股票详情面板 — 展示当前选中股票的全部行情数据
 */
class DetailPanel : public QWidget {
    Q_OBJECT

public:
    explicit DetailPanel(QWidget* parent = nullptr);

    /// 语言切换时刷新界面文本
    void retranslateUi();

    /// 收到新行情数据时更新显示
    void updateQuote(const datahub::QuoteData& quote);

    /// 显示策略体检结果（由外部编排层回填）
    /// @param ok       表达式是否解析/求值成功
    /// @param hit      条件是否命中（仅当 ok 且为布尔条件时有效）
    /// @param text     要显示的具体文本（含数值表达式的情况）
    void setCheckResult(bool ok, bool hit, const QString& text);

    /// 清空显示
    void clear();

signals:
    /// 用户在体检输入框按下回车或点击按钮时发出
    void strategyCheckRequested(const QString& expression);

private slots:
    void onCheckClicked();

private:
    QLabel* titleLabel_;
    QTableWidget* table_;
    QGroupBox* checkGroup_;
    QLineEdit* checkEdit_;
    QPushButton* btnCheck_;
    QLabel* checkResult_;
    QString currentSymbol_;
};

} // namespace fininsight::panels
