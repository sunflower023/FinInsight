#include "MainWindow.h"
#include "charts/KLineChart.h"
#include "datahub/DataHub.h"
#include "datahub/YahooProducer.h"
#include "datahub/QuoteData.h"
#include "panels/StockSearchBar.h"
#include "panels/StockListPanel.h"
#include "panels/DetailPanel.h"
#include "panels/PortfolioPanel.h"

#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>

// ── 全局样式表 — 清晰专业浅色主题 ──────────────────
// 原则：Restrained 配色 + solid 色(去AI味) + 对比度达标
static const char* kGlobalStyle = R"(
    QWidget {
        background-color: #ffffff;
        color: #1e1e1e;
        font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
        font-size: 13px;
    }

    /* 菜单栏 */
    QMenuBar {
        background-color: #f7f8fa;
        border-bottom: 1px solid #e0e0e0;
        padding: 2px 0;
        font-size: 13px;
    }
    QMenuBar::item {
        padding: 4px 10px;
        border-radius: 3px;
    }
    QMenuBar::item:selected {
        background-color: #e8f0fe;
        color: #1a73e8;
    }
    QMenu {
        background-color: #ffffff;
        border: 1px solid #dadce0;
        border-radius: 4px;
        padding: 4px 0;
        font-size: 13px;
    }
    QMenu::item {
        padding: 6px 32px 6px 16px;
    }
    QMenu::item:selected {
        background-color: #e8f0fe;
        color: #1a73e8;
    }
    QMenu::separator {
        height: 1px;
        background: #e0e0e0;
        margin: 4px 8px;
    }

    /* 状态栏 */
    QStatusBar {
        background-color: #f7f8fa;
        color: #5f6368;
        border-top: 1px solid #e0e0e0;
        font-size: 12px;
        padding: 2px 0;
    }

    /* 搜索框 */
    QLineEdit {
        background-color: #ffffff;
        border: 1px solid #dadce0;
        border-radius: 4px;
        padding: 8px 14px;
        color: #1e1e1e;
        font-size: 14px;
        selection-background-color: #c2dbfc;
    }
    QLineEdit:focus {
        border-color: #1a73e8;
        border-width: 2px;
        padding: 7px 13px;
    }

    /* 表格 */
    QTableWidget {
        background-color: #ffffff;
        alternate-background-color: #f8f9fa;
        gridline-color: #e8eaed;
        border: 1px solid #e0e0e0;
        border-radius: 2px;
        font-size: 12px;
    }
    QTableWidget::item {
        padding: 5px 10px;
        border-bottom: 1px solid #f0f0f0;
    }
    QTableWidget::item:selected {
        background-color: #e8f0fe;
        color: #1a73e8;
    }
    QHeaderView::section {
        background-color: #f7f8fa;
        color: #444d56;
        border: none;
        border-bottom: 1px solid #e0e0e0;
        padding: 7px 10px;
        font-weight: 600;
        font-size: 11px;
    }

    /* 列表面板 */
    QListWidget {
        background-color: #ffffff;
        alternate-background-color: #f8f9fa;
        border: 1px solid #e0e0e0;
        border-radius: 2px;
        outline: none;
        font-size: 13px;
    }
    QListWidget::item {
        padding: 7px 12px;
        border-bottom: 1px solid #f0f0f0;
    }
    QListWidget::item:selected {
        background-color: #e8f0fe;
        color: #1a73e8;
    }
    QListWidget::item:hover:!selected {
        background-color: #f1f3f4;
    }

    /* 按钮 — solid 色，无渐变 */
    QPushButton {
        background-color: #ffffff;
        border: 1px solid #dadce0;
        border-radius: 4px;
        padding: 7px 18px;
        color: #1e1e1e;
        font-weight: 500;
        font-size: 13px;
    }
    QPushButton:hover {
        background-color: #e8f0fe;
        border-color: #1a73e8;
        color: #1a73e8;
    }
    QPushButton:pressed {
        background-color: #d2e3fc;
    }

    /* 标签 — 提高对比度 */
    QLabel {
        color: #444d56;
        padding: 2px 4px;
        font-size: 13px;
    }

    /* 组合框 */
    QGroupBox {
        background-color: #ffffff;
        border: 1px solid #e0e0e0;
        border-radius: 4px;
        margin-top: 14px;
        padding: 20px 12px 12px;
        font-weight: 600;
        color: #1e1e1e;
        font-size: 13px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 12px;
        padding: 0 6px;
        color: #1a73e8;
        font-size: 13px;
    }

    /* 滚动条 */
    QScrollBar:vertical {
        background: transparent;
        width: 10px;
        margin: 2px;
    }
    QScrollBar::handle:vertical {
        background: #c4c7cc;
        border-radius: 5px;
        min-height: 30px;
    }
    QScrollBar::handle:vertical:hover {
        background: #9aa0a6;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0;
    }

    /* SpinBox */
    QSpinBox {
        background-color: #ffffff;
        border: 1px solid #dadce0;
        border-radius: 4px;
        padding: 5px 10px;
        color: #1e1e1e;
        font-size: 13px;
    }
    QSpinBox:focus {
        border-color: #1a73e8;
    }

    /* Dock 面板标签 — 原生渲染，不干预 */
)";

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("MainWindow");
    setWindowTitle("FinInsight");
    setMinimumSize(1024, 640);

    // 应用暗色主题
    qobject_cast<QApplication*>(QCoreApplication::instance())
        ->setStyleSheet(QLatin1String(kGlobalStyle));

    setupUi();
    yahoo_producer_ = new fininsight::datahub::YahooProducer(this);
    setupDataConnection("AAPL");
}

MainWindow::~MainWindow() {
    if (klineSubId_ >= 0) fininsight::datahub::DataHub::instance().unsubscribe(klineSubId_);
    if (quoteSubId_ >= 0) fininsight::datahub::DataHub::instance().unsubscribe(quoteSubId_);
}

// ═══════════════════════════════════════════════════════
//  UI 搭建
// ═══════════════════════════════════════════════════════

void MainWindow::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 菜单栏（作为布局子控件，不会和搜索栏重叠）
    setupMenuBar();
    layout->addWidget(menu_bar_);

    // 顶部搜索栏
    search_bar_ = new fininsight::panels::StockSearchBar();
    layout->addWidget(search_bar_);

    dock_manager_ = new ads::CDockManager(this);
    layout->addWidget(dock_manager_);

    setupPanels();

    // —— 底部状态栏 ——
    status_bar_ = new QStatusBar();
    status_symbol_ = new QLabel("AAPL");
    status_symbol_->setStyleSheet("color:#1a73e8; font-weight:bold; font-size:14px; padding:0 10px;");
    status_price_  = new QLabel("Loading...");
    status_price_->setStyleSheet("color:#1e1e1e; font-size:13px; padding:0 10px;");
    status_bar_->addWidget(status_symbol_);
    status_bar_->addWidget(status_price_);
    status_bar_->addPermanentWidget(new QLabel("FinInsight v1.0"));
    layout->addWidget(status_bar_);
}

void MainWindow::setupMenuBar()
{
    menu_bar_ = new QMenuBar(this);
    auto* file_menu = menu_bar_->addMenu("File");
    file_menu->addAction("Exit", this, &QWidget::close);

    auto* view_menu = menu_bar_->addMenu("View");
    view_menu->addAction("Reset Layout", [this]() {
        dock_manager_->restoreState(QByteArray());
    });
}

void MainWindow::setupPanels()
{
    // —— 左侧：自选股列表 ——
    stock_list_ = new fininsight::panels::StockListPanel();
    auto* list_dock = new ads::CDockWidget("  Watchlist");
    list_dock->setWidget(stock_list_);
    list_dock->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromContent);
    dock_manager_->addDockWidget(ads::LeftDockWidgetArea, list_dock);

    // —— 中间：K 线图 ——
    kline_chart_ = new fininsight::charts::KLineChart();
    auto* chart_dock = new ads::CDockWidget("  Chart");
    chart_dock->setWidget(kline_chart_);
    dock_manager_->addDockWidget(ads::CenterDockWidgetArea, chart_dock);

    // —— 右侧：详情 ——
    detail_panel_ = new fininsight::panels::DetailPanel();
    auto* detail_dock = new ads::CDockWidget("  Detail");
    detail_dock->setWidget(detail_panel_);
    dock_manager_->addDockWidget(ads::RightDockWidgetArea, detail_dock, chart_dock->dockAreaWidget());

    // —— 底部：投资组合 ——
    portfolio_ = new fininsight::panels::PortfolioPanel();
    auto* portfolio_dock = new ads::CDockWidget("  Portfolio");
    portfolio_dock->setWidget(portfolio_);
    dock_manager_->addDockWidget(ads::BottomDockWidgetArea, portfolio_dock, chart_dock->dockAreaWidget());

    // —— 信号连接 ——
    connect(search_bar_, &fininsight::panels::StockSearchBar::searchRequested,
            this, &MainWindow::onSearchRequested);
    connect(stock_list_, &fininsight::panels::StockListPanel::stockSelected,
            this, &MainWindow::loadStockData);
}

// ═══════════════════════════════════════════════════════
//  数据连接
// ═══════════════════════════════════════════════════════

void MainWindow::setupDataConnection(const QString& symbol)
{
    currentSymbol_ = symbol;

    // 取消旧订阅
    if (klineSubId_ >= 0) fininsight::datahub::DataHub::instance().unsubscribe(klineSubId_);
    if (quoteSubId_ >= 0) fininsight::datahub::DataHub::instance().unsubscribe(quoteSubId_);

    // 订阅 K 线
    klineSubId_ = fininsight::datahub::DataHub::instance().subscribe(
        symbol + ".kline.daily",
        [this](const QVariant& data) {
            auto bars = data.value<QVector<fininsight::datahub::KLineData>>();
            if (bars.isEmpty()) return;
            kline_chart_->setData(bars);
            kline_chart_->addMA(5,  QColor(255, 180, 50));
            kline_chart_->addMA(20, QColor(80, 160, 255));
            kline_chart_->addMA(60, QColor(180, 180, 180));
            kline_chart_->addBollinger();
        });

    // 订阅实时报价
    quoteSubId_ = fininsight::datahub::DataHub::instance().subscribe(
        symbol + ".quote",
        [this](const QVariant& data) {
            auto quote = data.value<fininsight::datahub::QuoteData>();
            detail_panel_->updateQuote(quote);
            stock_list_->updatePrice(quote.symbol, quote.price,
                                      quote.changePercent);
            // 更新状态栏
            if (status_symbol_) status_symbol_->setText(quote.symbol);
            if (status_price_) {
                QString changeStr = quote.change >= 0
                    ? QString("<span style='color:#e57373'>+%1%</span>")
                          .arg(quote.changePercent, 0, 'f', 2)
                    : QString("<span style='color:#81c784'>%1%</span>")
                          .arg(quote.changePercent, 0, 'f', 2);
                status_price_->setText(QString("$%1  %2")
                    .arg(quote.price, 0, 'f', 2).arg(changeStr));
                status_price_->setTextFormat(Qt::RichText);
            }
        });

    // 拉取数据
    yahoo_producer_->fetchOrCache(symbol);
    yahoo_producer_->fetchKLine(symbol, "6mo");
}

void MainWindow::onSearchRequested(const QString& symbol)
{
    stock_list_->addStock(symbol, symbol);
    loadStockData(symbol);
}

void MainWindow::loadStockData(const QString& symbol)
{
    setupDataConnection(symbol);
}
