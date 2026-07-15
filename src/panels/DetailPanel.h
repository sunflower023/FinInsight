#pragma once

#include "datahub/QuoteData.h"

#include <QWidget>
#include <QTableWidget>

namespace fininsight::panels {

/**
 * @brief 股票详情面板 — 展示当前选中股票的全部行情数据
 */
class DetailPanel : public QWidget {
    Q_OBJECT

public:
    explicit DetailPanel(QWidget* parent = nullptr);

    /// 收到新行情数据时更新显示
    void updateQuote(const datahub::QuoteData& quote);

    /// 清空显示
    void clear();

private:
    QTableWidget* table_;
};

} // namespace fininsight::panels
