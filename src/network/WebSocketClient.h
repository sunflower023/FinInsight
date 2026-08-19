#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>

class QTimer;
class QWebSocket;

namespace fininsight::network {

/**
 * Transport-only WebSocket client. Provider messages stay opaque; adapters
 * decide how to subscribe and how to normalize them into QuoteData.
 */
class WebSocketClient final : public QObject {
    Q_OBJECT

public:
    explicit WebSocketClient(QObject* parent = nullptr);
    ~WebSocketClient() override;

    void connectToUrl(const QUrl& url);
    void disconnectFromServer();
    bool isConnected() const;

    void sendText(const QString& message);
    void sendBinary(const QByteArray& message);

    void setHeartbeatInterval(int intervalMs);
    void setReconnectPolicy(bool enabled, int initialDelayMs = 500,
                            int maxDelayMs = 30000, int maxAttempts = -1);

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString& message);
    void binaryMessageReceived(const QByteArray& message);
    void errorOccurred(const QString& message);
    void reconnectScheduled(int delayMs, int attempt);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessage(const QString& message);
    void onBinaryMessage(const QByteArray& message);
    void onSocketError();
    void sendHeartbeat();
    void reconnect();

private:
    void openSocket();
    void stopTimers();
    void scheduleReconnect();

    QWebSocket* socket_;
    QTimer* heartbeatTimer_;
    QTimer* reconnectTimer_;
    QUrl url_;
    bool intentionalDisconnect_ = false;
    bool reconnectEnabled_ = true;
    int heartbeatIntervalMs_ = 15000;
    int initialDelayMs_ = 500;
    int maxDelayMs_ = 30000;
    int maxAttempts_ = -1;
    int reconnectAttempt_ = 0;
};

} // namespace fininsight::network
