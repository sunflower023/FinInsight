#include "MainWindow.h"

#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("FinInsight - Financial Data Terminal");
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupMenuBar();

    dock_manager_ = new ads::CDockManager(this);
    layout->addWidget(dock_manager_);

    setupPanels();
}

void MainWindow::setupMenuBar()
{
    auto *menu_bar = new QMenuBar(this);
    auto *file_menu = menu_bar->addMenu("File");
    file_menu->addAction("Exit", this, &QWidget::close);

    auto *view_menu = menu_bar->addMenu("View");
    view_menu->addAction("Reset Layout", [this]() {
        dock_manager_->restoreState(QByteArray());
    });
}

void MainWindow::setupPanels()
{
    auto *market_panel = new ads::CDockWidget("Market Watch");
    market_panel->setWidget(new QLabel("Market data will be displayed here"));
    dock_manager_->addDockWidget(ads::CenterDockWidgetArea, market_panel);

    auto *chart_panel = new ads::CDockWidget("Chart");
    chart_panel->setWidget(new QLabel("Chart will be displayed here"));
    dock_manager_->addDockWidget(ads::CenterDockWidgetArea, chart_panel, market_panel->dockAreaWidget());
}
