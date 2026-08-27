#include "notifications/NotificationService.h"

#include <QDateTime>
#include <algorithm>

namespace fininsight::notifications {

NotificationService::NotificationService(QObject* parent) : QObject(parent) {}

quint64 NotificationService::publish(Notification notification)
{
    if (notification.title.trimmed().isEmpty()) return 0;
    if (!notification.dedupKey.isEmpty()) {
        for (auto it = notifications_.crbegin(); it != notifications_.crend(); ++it) {
            if (it->dedupKey == notification.dedupKey && !it->read) return it->id;
        }
    }
    notification.id = nextId_++;
    if (notification.timestampMs <= 0) notification.timestampMs = QDateTime::currentMSecsSinceEpoch();
    notifications_.prepend(std::move(notification));
    while (notifications_.size() > capacity_) notifications_.removeLast();
    emit changed();
    return notifications_.front().id;
}

int NotificationService::unreadCount() const
{
    return int(std::count_if(notifications_.cbegin(), notifications_.cend(),
                             [](const Notification& notification) { return !notification.read; }));
}

void NotificationService::markAllRead()
{
    bool changedAny = false;
    for (auto& notification : notifications_) {
        if (!notification.read) { notification.read = true; changedAny = true; }
    }
    if (changedAny) emit changed();
}

} // namespace fininsight::notifications
