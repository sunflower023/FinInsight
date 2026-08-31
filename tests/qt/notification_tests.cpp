#include "notifications/NotificationService.h"
#include "notifications/TradingNotificationBridge.h"
#include "panels/NotificationPanel.h"

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTest>

class NotificationTests final : public QObject {
    Q_OBJECT
private slots:
    void deduplicatesAndMarksRead();
    void tradingBridgeAndPanel();
};

void NotificationTests::deduplicatesAndMarksRead()
{
    fininsight::notifications::NotificationService service;
    QSignalSpy changed(&service, &fininsight::notifications::NotificationService::changed);
    const auto first = service.publish({0, QStringLiteral("realtime"), QStringLiteral("Disconnected"),
                                        QStringLiteral("Stream unavailable"), QStringLiteral("stream-down"),
                                        fininsight::notifications::Severity::Warning});
    const auto duplicate = service.publish({0, QStringLiteral("realtime"), QStringLiteral("Disconnected"),
                                            QStringLiteral("Repeated"), QStringLiteral("stream-down"),
                                            fininsight::notifications::Severity::Warning});
    QVERIFY(first > 0);
    QCOMPARE(duplicate, first);
    QCOMPARE(service.notifications().size(), 1);
    QCOMPARE(service.unreadCount(), 1);
    QCOMPARE(changed.count(), 1);

    service.markAllRead();
    QCOMPARE(service.unreadCount(), 0);
    QCOMPARE(changed.count(), 2);
    const auto afterRead = service.publish({0, QStringLiteral("realtime"), QStringLiteral("Disconnected"),
                                            QStringLiteral("New incident"), QStringLiteral("stream-down"),
                                            fininsight::notifications::Severity::Warning});
    QVERIFY(afterRead != first);
    QCOMPARE(service.notifications().size(), 2);
}

void NotificationTests::tradingBridgeAndPanel()
{
    fininsight::notifications::NotificationService service;
    fininsight::notifications::TradingNotificationBridge bridge(service);
    fininsight::panels::NotificationPanel panel(service);
    auto* label = panel.findChild<QLabel*>(QStringLiteral("notificationUnreadLabel"));
    auto* table = panel.findChild<QTableWidget*>(QStringLiteral("notificationTable"));
    auto* markRead = panel.findChild<QPushButton*>(QStringLiteral("notificationMarkAllReadButton"));
    QVERIFY(label && table && markRead);

    fininsight::trading::Order rejected;
    rejected.request.clientOrderId = "paper-1";
    rejected.request.symbol = "AAPL";
    rejected.request.quantity = 5;
    rejected.status = fininsight::trading::OrderStatus::RiskRejected;
    rejected.rejectionReason = "Kill switch is enabled";
    bridge.onOrderChanged(rejected);

    QCOMPARE(service.notifications().size(), 1);
    QCOMPARE(service.notifications().front().severity, fininsight::notifications::Severity::Warning);
    QCOMPARE(table->rowCount(), 1);
    QVERIFY(table->item(0, 2)->text().contains(QStringLiteral("RiskRejected")));
    QCOMPARE(label->text(), QStringLiteral("Unread: 1"));
    markRead->click();
    QCOMPARE(label->text(), QStringLiteral("Unread: 0"));
}

QTEST_MAIN(NotificationTests)
#include "notification_tests.moc"
