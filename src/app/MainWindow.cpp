#include "MainWindow.h"
#include "charts/KLineChart.h"
#include "datahub/DataHub.h"
#include "datahub/YahooProducer.h"
#include "datahub/YahooWebSocketAdapter.h"
#include "datahub/QuoteStreamService.h"
#include "datahub/QuoteData.h"
#include "panels/StockSearchBar.h"
#include "panels/StockListPanel.h"
#include "panels/DetailPanel.h"
#include "panels/ExperimentPanel.h"
#include "panels/PortfolioPanel.h"
#include "panels/AgentReviewPanel.h"
#include "panels/RealtimeTradingPanel.h"
#include "panels/NotificationPanel.h"
#include "notifications/NotificationService.h"
#include "notifications/TradingNotificationBridge.h"
#include "monitoring/HealthMonitor.h"
#include "monitoring/LatencyTracker.h"
#include "panels/HealthPanel.h"
#include "analysis/BehaviorAnalyzer.h"
#include "storage/Database.h"
#include "storage/EvidenceSnapshotRepository.h"
#include "storage/TradingOrderRepository.h"
#include "core/I18n.h"

#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
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

    /* Qt ADS 标签标题：清除全局 QLabel 的 padding。
       否则 CElidingLabel 的 sizeHint 按纯文字宽度计算、渲染时又缩进 padding，
       会导致标题首尾被裁剪 */
    QLabel#dockWidgetTabLabel {
        padding: 0px;
        margin: 0px;
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

    evidenceRepository_ = std::make_unique<fininsight::storage::EvidenceSnapshotRepository>(
        fininsight::storage::Database::instance());
    tradingOrderRepository_ = std::make_unique<fininsight::storage::TradingOrderRepository>(
        fininsight::storage::Database::instance());
    notificationService_ = std::make_unique<fininsight::notifications::NotificationService>();
    tradingNotificationBridge_ = std::make_unique<fininsight::notifications::TradingNotificationBridge>(*notificationService_);
    healthMonitor_ = std::make_unique<fininsight::monitoring::HealthMonitor>();
    latencyTracker_ = std::make_unique<fininsight::monitoring::LatencyTracker>();
    setupUi();
    yahoo_producer_ = new fininsight::datahub::YahooProducer(this);
    yahoo_stream_ = new fininsight::datahub::YahooWebSocketAdapter(this);
    quote_stream_service_ = new fininsight::datahub::QuoteStreamService(
        yahoo_stream_, [this](const QString& symbol) { yahoo_producer_->fetchQuote(symbol); }, this);
    connect(quote_stream_service_, &fininsight::datahub::QuoteStreamService::stateChanged,
            this, [this](fininsight::datahub::QuoteStreamService::State state, const QString& detail) {
        if (healthMonitor_) healthMonitor_->onStreamState(state, detail);
        if (!status_stream_) return;
        using State = fininsight::datahub::QuoteStreamService::State;
        switch (state) {
        case State::Disabled: status_stream_->setText(I18n::instance().t("Realtime: off")); break;
        case State::Connecting: status_stream_->setText(I18n::instance().t("Realtime: connecting")); break;
        case State::Live: status_stream_->setText(I18n::instance().t("Realtime: live (experimental)")); break;
        case State::Fallback: status_stream_->setText(I18n::instance().t("Realtime: HTTP fallback")); break;
        case State::Stale: status_stream_->setText(I18n::instance().t("Realtime: stale / fallback")); break;
        case State::Disconnected: status_stream_->setText(I18n::instance().t("Realtime: disconnected")); break;
        }
        if (!notificationService_ || state == State::Disabled || state == State::Connecting) return;
        const bool degraded = state == State::Fallback || state == State::Stale || state == State::Disconnected;
        notificationService_->publish({0, QStringLiteral("realtime"),
            degraded ? QStringLiteral("Realtime quote degraded") : QStringLiteral("Realtime quote live"),
            state == State::Fallback ? QStringLiteral("Using HTTP quote fallback") :
            state == State::Stale ? QStringLiteral("Realtime quote is stale") :
            state == State::Disconnected ? QStringLiteral("Realtime quote stream disconnected") :
            QStringLiteral("Realtime quote stream recovered"),
            QStringLiteral("realtime-state:%1").arg(int(state)),
            degraded ? fininsight::notifications::Severity::Warning : fininsight::notifications::Severity::Info});
    });
    connect(quote_stream_service_, &fininsight::datahub::QuoteStreamService::quoteAccepted, this, [this](const fininsight::datahub::QuoteData& quote) {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (healthMonitor_) healthMonitor_->onQuoteAccepted(now);
        const qint64 sourceMs = quote.timestamp < 100000000000LL ? quote.timestamp * 1000 : quote.timestamp;
        if (latencyTracker_ && sourceMs > 0 && now >= sourceMs) latencyTracker_->record("quote_delivery", double(now-sourceMs));
    });
    connect(quote_stream_service_, &fininsight::datahub::QuoteStreamService::quoteRejected, this, [this](const QString& reason) { if (healthMonitor_) healthMonitor_->onQuoteRejected(reason); });
    setupDataConnection("AAPL");

    // —— 默认接入实时行情：启动 WebSocket 流（断线时自动回退 HTTP）——
    if (quote_stream_service_) quote_stream_service_->setEnabled(true);

    // —— 语言切换：刷新主窗口 + 所有面板 ——
    connect(&I18n::instance(), &I18n::languageChanged, this, [this] {
        retranslateUi();
        if (search_bar_) search_bar_->retranslateUi();
        if (stock_list_) stock_list_->retranslateUi();
        if (detail_panel_) detail_panel_->retranslateUi();
        if (portfolio_) portfolio_->retranslateUi();
        if (experiment_) experiment_->retranslateUi();
        if (agent_review_) agent_review_->retranslateUi();
        if (realtime_trading_) realtime_trading_->retranslateUi();
        if (notification_panel_) notification_panel_->retranslateUi();
        if (health_panel_) health_panel_->retranslateUi();
    });
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

    // 菜单栏
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
    status_price_  = new QLabel(I18n::instance().t("Loading..."));
    status_price_->setStyleSheet("color:#1e1e1e; font-size:13px; padding:0 10px;");
    status_bar_->addWidget(status_symbol_);
    status_bar_->addWidget(status_price_);
    status_stream_ = new QLabel(I18n::instance().t("Realtime: off"));
    status_stream_->setObjectName(QStringLiteral("streamStatusLabel"));
    status_stream_->setStyleSheet("color:#777; padding:0 10px;");
    status_bar_->addPermanentWidget(status_stream_);
    status_bar_->addPermanentWidget(new QLabel("FinInsight v1.0"));
    layout->addWidget(status_bar_);
}

void MainWindow::setupMenuBar()
{
    menu_bar_ = new QMenuBar(this);

    file_menu_ = menu_bar_->addMenu(I18n::instance().t("File"));
    exit_action_ = file_menu_->addAction(I18n::instance().t("Exit"), this, &QWidget::close);

    view_menu_ = menu_bar_->addMenu(I18n::instance().t("View"));
    reset_layout_action_ = view_menu_->addAction(I18n::instance().t("Reset Layout"), [this]() {
        dock_manager_->restoreState(QByteArray());
    });

    // —— 语言切换菜单 ——
    language_menu_ = menu_bar_->addMenu(I18n::instance().t("Language"));
    auto* language_group = new QActionGroup(this);
    language_group->setExclusive(true);

    language_english_action_ = language_menu_->addAction("English");
    language_english_action_->setCheckable(true);
    language_group->addAction(language_english_action_);

    language_chinese_action_ = language_menu_->addAction("中文");
    language_chinese_action_->setCheckable(true);
    language_group->addAction(language_chinese_action_);

    const bool chinese = I18n::instance().isChinese();
    language_chinese_action_->setChecked(chinese);
    language_english_action_->setChecked(!chinese);

    connect(language_english_action_, &QAction::triggered, this, [this] {
        I18n::instance().setLanguage(I18n::Language::English);
    });
    connect(language_chinese_action_, &QAction::triggered, this, [this] {
        I18n::instance().setLanguage(I18n::Language::Chinese);
    });
}

void MainWindow::setupPanels()
{
    // —— 左侧：自选股列表 ——
    stock_list_ = new fininsight::panels::StockListPanel();
    list_dock_ = new ads::CDockWidget(I18n::instance().t("Watchlist"));
    list_dock_->setWidget(stock_list_);
    list_dock_->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromContent);
    dock_manager_->addDockWidget(ads::LeftDockWidgetArea, list_dock_);

    // —— 中间：K 线图 ——
    kline_chart_ = new fininsight::charts::KLineChart();
    chart_dock_ = new ads::CDockWidget(I18n::instance().t("Chart"));
    chart_dock_->setWidget(kline_chart_);
    dock_manager_->addDockWidget(ads::CenterDockWidgetArea, chart_dock_);

    // —— 右侧：详情 ——
    detail_panel_ = new fininsight::panels::DetailPanel();
    detail_dock_ = new ads::CDockWidget(I18n::instance().t("Detail"));
    detail_dock_->setWidget(detail_panel_);
    dock_manager_->addDockWidget(ads::RightDockWidgetArea, detail_dock_, chart_dock_->dockAreaWidget());

    // —— 底部：投资组合 ——
    portfolio_ = new fininsight::panels::PortfolioPanel();
    portfolio_dock_ = new ads::CDockWidget(I18n::instance().t("Simulation Portfolio"));
    portfolio_dock_->setWidget(portfolio_);
    auto* simulation_area = dock_manager_->addDockWidget(
        ads::BottomDockWidgetArea, portfolio_dock_, chart_dock_->dockAreaWidget());

    experiment_ = new fininsight::panels::ExperimentPanel();
    experiment_dock_ = new ads::CDockWidget(I18n::instance().t("Historical Experiment"));
    experiment_dock_->setWidget(experiment_);
    dock_manager_->addDockWidgetTabToArea(experiment_dock_, simulation_area);
    agent_review_ = new fininsight::panels::AgentReviewPanel(*evidenceRepository_);
    agent_dock_ = new ads::CDockWidget(I18n::instance().t("Agent Review"));
    agent_dock_->setWidget(agent_review_);
    dock_manager_->addDockWidgetTabToArea(agent_dock_, simulation_area);
    realtime_trading_ = new fininsight::panels::RealtimeTradingPanel();
    connect(realtime_trading_, &fininsight::panels::RealtimeTradingPanel::orderChanged,
            this, [this](const fininsight::trading::Order& order) {
        if (tradingOrderRepository_) tradingOrderRepository_->save(order, "paper");
        if (tradingNotificationBridge_) tradingNotificationBridge_->onOrderChanged(order);
    });
    trading_dock_ = new ads::CDockWidget(I18n::instance().t("Realtime Paper Trading"));
    trading_dock_->setWidget(realtime_trading_);
    dock_manager_->addDockWidgetTabToArea(trading_dock_, simulation_area);
    notification_panel_ = new fininsight::panels::NotificationPanel(*notificationService_);
    notification_dock_ = new ads::CDockWidget(I18n::instance().t("Notifications"));
    notification_dock_->setWidget(notification_panel_);
    dock_manager_->addDockWidgetTabToArea(notification_dock_, simulation_area);
    health_panel_ = new fininsight::panels::HealthPanel(*healthMonitor_, *latencyTracker_);
    health_dock_ = new ads::CDockWidget(I18n::instance().t("Health"));
    health_dock_->setWidget(health_panel_);
    dock_manager_->addDockWidgetTabToArea(health_dock_, simulation_area);
    portfolio_dock_->setAsCurrentTab();

    // —— 信号连接 ——
    connect(search_bar_, &fininsight::panels::StockSearchBar::searchRequested,
            this, &MainWindow::onSearchRequested);
    connect(stock_list_, &fininsight::panels::StockListPanel::stockSelected,
            this, &MainWindow::loadStockData);
    connect(portfolio_, &fininsight::panels::PortfolioPanel::evidenceChanged, this, [this] {
        persistEvidence(portfolio_->evidenceSnapshot());
    });

    // —— K 线选时点 → 组合面板历史价联动 ——
    connect(kline_chart_, &fininsight::charts::KLineChart::barSelected,
            this, [this](int, const fininsight::datahub::KLineData& bar) {
        portfolio_->setHistoricalBar(bar);
    });
    connect(kline_chart_, &fininsight::charts::KLineChart::barHovered,
            this, [this](int, const fininsight::datahub::KLineData& bar) {
        portfolio_->previewAtPrice(bar.close);
    });
    connect(kline_chart_, &fininsight::charts::KLineChart::hoverLeft,
            this, [this] { portfolio_->previewAtPrice(0.0); });
    // 成交后回传 K 线画买卖标记
    connect(portfolio_, &fininsight::panels::PortfolioPanel::tradeExecuted,
            this, [this](const QString& date, bool isBuy) {
        kline_chart_->addTradeMarker(date, isBuy);
    });
    // 点击成交记录 → K 线跳转对应时点
    connect(portfolio_, &fininsight::panels::PortfolioPanel::tradeSelected,
            this, [this](const QString& date) {
        kline_chart_->highlightDate(date);
    });

    data_menu_ = menu_bar_->addMenu(I18n::instance().t("Data"));
    realtime_action_ = data_menu_->addAction(I18n::instance().t("Experimental Realtime Quotes"));
    realtime_action_->setCheckable(true);
    realtime_action_->setChecked(true);
    connect(realtime_action_, &QAction::toggled, this, [this](bool enabled) {
        if (quote_stream_service_) quote_stream_service_->setEnabled(enabled);
    });
    connect(experiment_, &fininsight::panels::ExperimentPanel::evidenceChanged, this, [this] {
        persistEvidence(experiment_->evidenceSnapshot());
    });
}

void MainWindow::persistEvidence(const fininsight::analysis::EvidenceSnapshot& evidence)
{
    if (!evidenceRepository_ || evidence.trades.empty()) return;
    const auto report = fininsight::analysis::analyzeBehavior(evidence);
    evidenceRepository_->save(evidence, report);
    if (agent_review_) agent_review_->refresh();
}

// ═══════════════════════════════════════════════════════
//  数据连接
// ═══════════════════════════════════════════════════════

void MainWindow::setupDataConnection(const QString& symbol)
{
    currentSymbol_ = symbol;
    if (quote_stream_service_) quote_stream_service_->setSymbol(currentSymbol_);
    portfolio_->setCurrentSymbol(currentSymbol_);
    experiment_->setCurrentSymbol(currentSymbol_);
    realtime_trading_->setCurrentSymbol(currentSymbol_);

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
            experiment_->setHistoricalData(currentSymbol_, bars);
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
            portfolio_->onQuoteUpdated(quote);
            realtime_trading_->onQuoteUpdated(quote);
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

void MainWindow::retranslateUi()
{
    // 菜单
    if (file_menu_) file_menu_->setTitle(I18n::instance().t("File"));
    if (view_menu_) view_menu_->setTitle(I18n::instance().t("View"));
    if (data_menu_) data_menu_->setTitle(I18n::instance().t("Data"));
    if (language_menu_) language_menu_->setTitle(I18n::instance().t("Language"));
    if (exit_action_) exit_action_->setText(I18n::instance().t("Exit"));
    if (reset_layout_action_) reset_layout_action_->setText(I18n::instance().t("Reset Layout"));
    if (realtime_action_) realtime_action_->setText(I18n::instance().t("Experimental Realtime Quotes"));

    // Dock 面板标题（同时修复标题前导空格导致的显示不全）
    if (list_dock_) list_dock_->setWindowTitle(I18n::instance().t("Watchlist"));
    if (chart_dock_) chart_dock_->setWindowTitle(I18n::instance().t("Chart"));
    if (detail_dock_) detail_dock_->setWindowTitle(I18n::instance().t("Detail"));
    if (portfolio_dock_) portfolio_dock_->setWindowTitle(I18n::instance().t("Simulation Portfolio"));
    if (experiment_dock_) experiment_dock_->setWindowTitle(I18n::instance().t("Historical Experiment"));
    if (agent_dock_) agent_dock_->setWindowTitle(I18n::instance().t("Agent Review"));
    if (trading_dock_) trading_dock_->setWindowTitle(I18n::instance().t("Realtime Paper Trading"));
    if (notification_dock_) notification_dock_->setWindowTitle(I18n::instance().t("Notifications"));
    if (health_dock_) health_dock_->setWindowTitle(I18n::instance().t("Health"));
}
