#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace fininsight::notifications {

enum class Severity { Info, Warning, Critical };

struct Notification {
    quint64 id = 0;
    QString source;
    QString title;
    QString message;
    QString dedupKey;
    Severity severity = Severity::Info;
    qint64 timestampMs = 0;
    bool read = false;
};

class NotificationService final : public QObject {
    Q_OBJECT
public:
    explicit NotificationService(QObject* parent = nullptr);

    quint64 publish(Notification notification);
    const QVector<Notification>& notifications() const { return notifications_; }
    int unreadCount() const;
    void markAllRead();

signals:
    void changed();

private:
    QVector<Notification> notifications_;
    quint64 nextId_ = 1;
    qsizetype capacity_ = 200;
};

} // namespace fininsight::notifications
