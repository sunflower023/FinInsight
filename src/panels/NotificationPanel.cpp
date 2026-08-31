#include "panels/NotificationPanel.h"
#include "core/I18n.h"
#include "notifications/NotificationService.h"

#include <QDateTime>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace fininsight::panels {

NotificationPanel::NotificationPanel(notifications::NotificationService& service, QWidget* parent)
    : QWidget(parent), service_(service)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    count_ = new QLabel;
    count_->setObjectName(QStringLiteral("notificationUnreadLabel"));
    markReadButton_ = new QPushButton;
    markReadButton_->setObjectName(QStringLiteral("notificationMarkAllReadButton"));
    table_ = new QTableWidget(0, 4);
    table_->setObjectName(QStringLiteral("notificationTable"));
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(count_);
    root->addWidget(markReadButton_);
    root->addWidget(table_, 1);
    connect(markReadButton_, &QPushButton::clicked, &service_, &notifications::NotificationService::markAllRead);
    connect(&service_, &notifications::NotificationService::changed, this, &NotificationPanel::refresh);
    retranslateUi();
    refresh();
}

void NotificationPanel::retranslateUi()
{
    markReadButton_->setText(I18n::instance().t("Mark All Read"));
    table_->setHorizontalHeaderLabels({I18n::instance().t("Time"), I18n::instance().t("Source"),
                                       I18n::instance().t("Event"), I18n::instance().t("Message")});
}

void NotificationPanel::refresh()
{
    count_->setText(I18n::instance().t("Unread: %1").arg(service_.unreadCount()));
    table_->setRowCount(0);
    for (const auto& notification : service_.notifications()) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(notification.timestampMs).toString(QStringLiteral("HH:mm:ss"))));
        table_->setItem(row, 1, new QTableWidgetItem(notification.source));
        table_->setItem(row, 2, new QTableWidgetItem(notification.title));
        table_->setItem(row, 3, new QTableWidgetItem(notification.message));
    }
}

} // namespace fininsight::panels
