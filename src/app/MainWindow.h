#pragma once

#include <QWidget>
#include <DockManager.h>

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void setupMenuBar();
    void setupPanels();

    ads::CDockManager *dock_manager_ = nullptr;
};
