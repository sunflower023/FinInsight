#pragma once

#include "analysis/BehaviorAnalyzer.h"

#include <QWidget>
#include <DockManager.h>
#include <memory>

class QMenuBar;
class QStatusBar;
class QLabel;
class QMenu;
class QAction;
namespace ads { class CDockWidget; }

namespace fininsight::charts {
class KLineChart;
}
namespace fininsight::datahub {
class QuoteStreamService;
class YahooWebSocketAdapter;
class YahooProducer;
}
namespace fininsight::panels {
class StockSearchBar;
class StockListPanel;
class DetailPanel;
class PortfolioPanel;
class ExperimentPanel;
class AgentReviewPanel;
class RealtimeTradingPanel;
class NotificationPanel;
class HealthPanel;
}
namespace fininsight::notifications { class NotificationService; class TradingNotificationBridge; }
namespace fininsight::monitoring { class HealthMonitor; class LatencyTracker; }
namespace fininsight::storage { class EvidenceSnapshotRepository; class TradingOrderRepository; }

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSearchRequested(const QString& symbol);

private:
    void setupUi();
    void setupMenuBar();
    void setupPanels();
    void setupDataConnection(const QString& symbol = "AAPL");
    void loadStockData(const QString& symbol);
    void persistEvidence(const fininsight::analysis::EvidenceSnapshot& evidence);
    void retranslateUi();

    ads::CDockManager *dock_manager_ = nullptr;
    QMenuBar* menu_bar_ = nullptr;
    QStatusBar* status_bar_ = nullptr;
    QLabel* status_symbol_ = nullptr;
    QLabel* status_price_  = nullptr;
    QLabel* status_stream_ = nullptr;

    // —— 菜单与语言切换（保留指针以便运行时切换文本）——
    QMenu* file_menu_ = nullptr;
    QMenu* view_menu_ = nullptr;
    QMenu* data_menu_ = nullptr;
    QMenu* language_menu_ = nullptr;
    QAction* exit_action_ = nullptr;
    QAction* reset_layout_action_ = nullptr;
    QAction* realtime_action_ = nullptr;
    QAction* language_english_action_ = nullptr;
    QAction* language_chinese_action_ = nullptr;

    // —— Dock 面板（保留指针以便运行时切换标题）——
    ads::CDockWidget* list_dock_ = nullptr;
    ads::CDockWidget* chart_dock_ = nullptr;
    ads::CDockWidget* detail_dock_ = nullptr;
    ads::CDockWidget* portfolio_dock_ = nullptr;
    ads::CDockWidget* experiment_dock_ = nullptr;
    ads::CDockWidget* agent_dock_ = nullptr;
    ads::CDockWidget* trading_dock_ = nullptr;
    ads::CDockWidget* notification_dock_ = nullptr;
    ads::CDockWidget* health_dock_ = nullptr;

    fininsight::charts::KLineChart*    kline_chart_    = nullptr;
    fininsight::datahub::YahooProducer* yahoo_producer_ = nullptr;
    fininsight::datahub::YahooWebSocketAdapter* yahoo_stream_ = nullptr;
    fininsight::datahub::QuoteStreamService* quote_stream_service_ = nullptr;
    fininsight::panels::StockSearchBar* search_bar_     = nullptr;
    fininsight::panels::StockListPanel* stock_list_     = nullptr;
    fininsight::panels::DetailPanel*    detail_panel_   = nullptr;
    fininsight::panels::PortfolioPanel* portfolio_      = nullptr;
    fininsight::panels::ExperimentPanel* experiment_   = nullptr;
    fininsight::panels::AgentReviewPanel* agent_review_ = nullptr;
    fininsight::panels::RealtimeTradingPanel* realtime_trading_ = nullptr;
    fininsight::panels::NotificationPanel* notification_panel_ = nullptr;
    fininsight::panels::HealthPanel* health_panel_ = nullptr;

    QString currentSymbol_;
    int klineSubId_ = -1;
    int quoteSubId_ = -1;
    std::unique_ptr<fininsight::storage::EvidenceSnapshotRepository> evidenceRepository_;
    std::unique_ptr<fininsight::storage::TradingOrderRepository> tradingOrderRepository_;
    std::unique_ptr<fininsight::notifications::NotificationService> notificationService_;
    std::unique_ptr<fininsight::notifications::TradingNotificationBridge> tradingNotificationBridge_;
    std::unique_ptr<fininsight::monitoring::HealthMonitor> healthMonitor_;
    std::unique_ptr<fininsight::monitoring::LatencyTracker> latencyTracker_;
};
