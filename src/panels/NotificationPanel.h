#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
namespace fininsight::notifications { class NotificationService; }

namespace fininsight::panels {

class NotificationPanel final : public QWidget {
    Q_OBJECT
public:
    explicit NotificationPanel(notifications::NotificationService& service, QWidget* parent = nullptr);

    /// 语言切换时刷新界面文本
    void retranslateUi();

private slots:
    void refresh();

private:
    notifications::NotificationService& service_;
    QLabel* count_ = nullptr;
    QPushButton* markReadButton_ = nullptr;
    QTableWidget* table_ = nullptr;
};

} // namespace fininsight::panels
