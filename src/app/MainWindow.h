#pragma once

#include <QWidget>
#include <DockManager.h>

class QMenuBar;
class QStatusBar;
class QLabel;

namespace fininsight::charts {
class KLineChart;
}
namespace fininsight::panels {
class StockSearchBar;
class StockListPanel;
class DetailPanel;
class PortfolioPanel;
}

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

    ads::CDockManager *dock_manager_ = nullptr;
    QMenuBar* menu_bar_ = nullptr;
    QStatusBar* status_bar_ = nullptr;
    QLabel* status_symbol_ = nullptr;
    QLabel* status_price_  = nullptr;
    fininsight::charts::KLineChart*    kline_chart_    = nullptr;
    fininsight::panels::StockSearchBar* search_bar_     = nullptr;
    fininsight::panels::StockListPanel* stock_list_     = nullptr;
    fininsight::panels::DetailPanel*    detail_panel_   = nullptr;
    fininsight::panels::PortfolioPanel* portfolio_      = nullptr;

    QString currentSymbol_;
    int klineSubId_ = -1;
    int quoteSubId_ = -1;
};
