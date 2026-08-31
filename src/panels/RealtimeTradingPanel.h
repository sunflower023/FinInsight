#pragma once
#include "datahub/QuoteData.h"
#include "trading/TradingTypes.h"
#include <QWidget>
#include <memory>
class QLabel; class QSpinBox; class QDoubleSpinBox; class QComboBox; class QPushButton; class QCheckBox; class QTableWidget; class QFormLayout;
namespace fininsight::trading { class PaperExecutionGateway; class OrderService; }
namespace fininsight::panels {
class RealtimeTradingPanel final : public QWidget {
    Q_OBJECT
public:
    explicit RealtimeTradingPanel(QWidget* parent = nullptr);
    ~RealtimeTradingPanel() override;
    /// 语言切换时刷新界面文本
    void retranslateUi();
    void setCurrentSymbol(const QString& symbol);
    void onQuoteUpdated(const datahub::QuoteData& quote);
signals:
    void orderChanged(const fininsight::trading::Order& order);
private slots:
    void submitOrder();
    void cancelSelected();
    void refresh();
private:
    QString currentSymbol_; datahub::QuoteData quote_; quint64 nextOrderId_ = 1;
    std::unique_ptr<trading::PaperExecutionGateway> gateway_;
    std::unique_ptr<trading::OrderService> orderService_;
    QLabel* environment_ = nullptr; QLabel* account_ = nullptr; QLabel* quoteStatus_ = nullptr; QLabel* message_ = nullptr;
    QSpinBox* quantity_ = nullptr; QDoubleSpinBox* limitPrice_ = nullptr; QComboBox* side_ = nullptr; QComboBox* type_ = nullptr;
    QPushButton* submit_ = nullptr; QPushButton* cancel_ = nullptr; QCheckBox* killSwitch_ = nullptr; QTableWidget* orders_ = nullptr;
    QFormLayout* form_ = nullptr;
};
} // namespace fininsight::panels
