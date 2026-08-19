#include "network/WebSocketClient.h"

#include <QTimer>
#include <QWebSocket>

namespace fininsight::network {

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent),
      socket_(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
      heartbeatTimer_(new QTimer(this)),
      reconnectTimer_(new QTimer(this))
{
    heartbeatTimer_->setSingleShot(false);
    reconnectTimer_->setSingleShot(true);
    connect(socket_, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(socket_, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(socket_, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessage);
    connect(socket_, &QWebSocket::binaryMessageReceived,
            this, &WebSocketClient::onBinaryMessage);
    connect(socket_, &QWebSocket::errorOccurred,
            this, &WebSocketClient::onSocketError);
    connect(heartbeatTimer_, &QTimer::timeout, this, &WebSocketClient::sendHeartbeat);
    connect(reconnectTimer_, &QTimer::timeout, this, &WebSocketClient::reconnect);
}

WebSocketClient::~WebSocketClient() {
    intentionalDisconnect_ = true;
    stopTimers();
    socket_->close();
}

void WebSocketClient::connectToUrl(const QUrl& url) {
    url_ = url;
    intentionalDisconnect_ = false;
    reconnectAttempt_ = 0;
    reconnectTimer_->stop();
    if (socket_->state() == QAbstractSocket::ConnectedState ||
        socket_->state() == QAbstractSocket::ConnectingState) {
        socket_->close();
    }
    openSocket();
}

void WebSocketClient::disconnectFromServer() {
    intentionalDisconnect_ = true;
    reconnectAttempt_ = 0;
    stopTimers();
    socket_->close();
}

bool WebSocketClient::isConnected() const {
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::sendText(const QString& message) {
    if (isConnected()) socket_->sendTextMessage(message);
}

void WebSocketClient::sendBinary(const QByteArray& message) {
    if (isConnected()) socket_->sendBinaryMessage(message);
}

void WebSocketClient::setHeartbeatInterval(int intervalMs) {
    heartbeatIntervalMs_ = qMax(0, intervalMs);
    if (heartbeatTimer_->isActive()) {
        heartbeatTimer_->stop();
        if (heartbeatIntervalMs_ > 0) heartbeatTimer_->start(heartbeatIntervalMs_);
    }
}

void WebSocketClient::setReconnectPolicy(bool enabled, int initialDelayMs,
                                          int maxDelayMs, int maxAttempts) {
    reconnectEnabled_ = enabled;
    initialDelayMs_ = qMax(0, initialDelayMs);
    maxDelayMs_ = qMax(initialDelayMs_, maxDelayMs);
    maxAttempts_ = maxAttempts;
    if (!enabled) reconnectTimer_->stop();
}

void WebSocketClient::onConnected() {
    reconnectAttempt_ = 0;
    if (heartbeatIntervalMs_ > 0) heartbeatTimer_->start(heartbeatIntervalMs_);
    emit connected();
}

void WebSocketClient::onDisconnected() {
    heartbeatTimer_->stop();
    emit disconnected();
    if (!intentionalDisconnect_) scheduleReconnect();
}

void WebSocketClient::onTextMessage(const QString& message) {
    emit textMessageReceived(message);
}

void WebSocketClient::onBinaryMessage(const QByteArray& message) {
    emit binaryMessageReceived(message);
}

void WebSocketClient::onSocketError() {
    emit errorOccurred(socket_->errorString());
}

void WebSocketClient::sendHeartbeat() {
    if (isConnected()) socket_->ping();
}

void WebSocketClient::reconnect() {
    if (!intentionalDisconnect_ && !url_.isEmpty()) openSocket();
}

void WebSocketClient::openSocket() {
    if (url_.isEmpty()) return;
    socket_->open(url_);
}

void WebSocketClient::stopTimers() {
    heartbeatTimer_->stop();
    reconnectTimer_->stop();
}

void WebSocketClient::scheduleReconnect() {
    if (!reconnectEnabled_ || url_.isEmpty()) return;
    if (maxAttempts_ >= 0 && reconnectAttempt_ >= maxAttempts_) return;
    ++reconnectAttempt_;
    const int shift = qMin(reconnectAttempt_ - 1, 30);
    const int delay = qMin(maxDelayMs_, initialDelayMs_ * (1 << shift));
    reconnectTimer_->start(delay);
    emit reconnectScheduled(delay, reconnectAttempt_);
}

} // namespace fininsight::network
