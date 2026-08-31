#pragma once

#include <QWidget>
class QLabel;
class QTimer;
namespace fininsight::monitoring { class HealthMonitor; class LatencyTracker; }
namespace fininsight::panels {
class HealthPanel final : public QWidget {
    Q_OBJECT
public:
    HealthPanel(monitoring::HealthMonitor& health, monitoring::LatencyTracker& latency, QWidget* parent = nullptr);

    /// 语言切换时刷新界面文本
    void retranslateUi();

private slots:
    void refresh();
private:
    monitoring::HealthMonitor& health_;
    monitoring::LatencyTracker& latency_;
    QLabel* status_ = nullptr;
    QLabel* counters_ = nullptr;
    QLabel* latencyLabel_ = nullptr;
    QTimer* timer_ = nullptr;
};
}
