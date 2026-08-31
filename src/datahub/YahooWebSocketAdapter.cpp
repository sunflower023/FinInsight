#include "datahub/YahooWebSocketAdapter.h"

#include "network/WebSocketClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cstring>

namespace fininsight::datahub {

namespace {
const QUrl kDefaultEndpoint(QStringLiteral("wss://streamer.finance.yahoo.com/"));

bool varint(const QByteArray& data, int& pos, quint64& value) {
    value = 0;
    int shift = 0;
    while (pos < data.size() && shift <= 63) {
        const quint8 byte = static_cast<quint8>(data.at(pos++));
        value |= quint64(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) return true;
        shift += 7;
    }
    return false;
}

bool bytes(const QByteArray& data, int& pos, QByteArray& value) {
    quint64 length = 0;
    if (!varint(data, pos, length) || length > quint64(data.size() - pos)) return false;
    value = data.mid(pos, static_cast<int>(length));
    pos += static_cast<int>(length);
    return true;
}

bool real64(const QByteArray& data, int& pos, double& value) {
    if (pos + 8 > data.size()) return false;
    std::memcpy(&value, data.constData() + pos, sizeof(value));
    pos += 8;
    return true;
}

bool real32(const QByteArray& data, int& pos, float& value) {
    if (pos + 4 > data.size()) return false;
    std::memcpy(&value, data.constData() + pos, sizeof(value));
    pos += 4;
    return true;
}

// protobuf sint64 使用 zigzag 编码，需解码回有符号整数
qint64 zigzag64(quint64 value) {
    return static_cast<qint64>((value >> 1) ^ -static_cast<qint64>(value & 1));
}
}

YahooWebSocketAdapter::YahooWebSocketAdapter(QObject* parent)
    : YahooWebSocketAdapter(kDefaultEndpoint, parent) {}

YahooWebSocketAdapter::YahooWebSocketAdapter(const QUrl& endpoint, QObject* parent)
    : QuoteStream(parent), endpoint_(endpoint), client_(new network::WebSocketClient(this)) {
    client_->setHeartbeatInterval(15000);
    client_->setReconnectPolicy(true, 500, 30000, -1);
    connect(client_, &network::WebSocketClient::connected, this, &YahooWebSocketAdapter::onConnected);
    connect(client_, &network::WebSocketClient::binaryMessageReceived, this, &YahooWebSocketAdapter::onBinaryMessage);
    connect(client_, &network::WebSocketClient::textMessageReceived, this, &YahooWebSocketAdapter::onTextMessage);
    connect(client_, &network::WebSocketClient::errorOccurred, this, &YahooWebSocketAdapter::onError);
    connect(client_, &network::WebSocketClient::disconnected, this, [this] { emit statusChanged(QStringLiteral("disconnected")); });
}

void YahooWebSocketAdapter::start() { client_->connectToUrl(endpoint_); emit statusChanged(QStringLiteral("connecting")); }
void YahooWebSocketAdapter::stop() { client_->disconnectFromServer(); emit statusChanged(QStringLiteral("stopped")); }

void YahooWebSocketAdapter::subscribe(const QString& symbol) {
    const QString value = normalize(symbol);
    if (value.isEmpty() || symbols_.contains(value)) return;
    symbols_.insert(value);
    if (client_->isConnected()) sendSymbols(QStringLiteral("subscribe"), {value});
}

void YahooWebSocketAdapter::unsubscribe(const QString& symbol) {
    const QString value = normalize(symbol);
    if (!symbols_.remove(value)) return;
    if (client_->isConnected()) sendSymbols(QStringLiteral("unsubscribe"), {value});
}

bool YahooWebSocketAdapter::isConnected() const { return client_->isConnected(); }

void YahooWebSocketAdapter::onConnected() {
    emit statusChanged(QStringLiteral("connected"));
    if (!symbols_.isEmpty()) sendSymbols(QStringLiteral("subscribe"), symbols_.values());
}

void YahooWebSocketAdapter::onBinaryMessage(const QByteArray& message) {
    QuoteData quote;
    if (!parsePricingData(message, quote)) return;
    emit quoteReceived(quote);
}

void YahooWebSocketAdapter::onTextMessage(const QString& message) {
    // Yahoo streamer 目前把 Protobuf 二进制用 Base64 编码后作为文本消息发送，
    // 需先解码回二进制再解析（否则收不到报价、状态一直停在「连接中」）。
    const QByteArray raw = QByteArray::fromBase64(message.toUtf8());
    QuoteData quote;
    if (!parsePricingData(raw, quote)) return;
    emit quoteReceived(quote);
}

void YahooWebSocketAdapter::onError(const QString& error) { emit errorOccurred(error); }

void YahooWebSocketAdapter::sendSymbols(const QString& action, const QStringList& symbols) {
    QJsonObject request;
    request[action] = QJsonArray::fromStringList(symbols);
    client_->sendText(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

bool YahooWebSocketAdapter::parsePricingData(const QByteArray& data, QuoteData& quote) {
    int pos = 0;
    while (pos < data.size()) {
        quint64 tag = 0;
        if (!varint(data, pos, tag) || tag == 0) return false;
        const int field = static_cast<int>(tag >> 3);
        const int wire = static_cast<int>(tag & 7);
        if (wire == 1) {
            // 64-bit double（兼容旧协议 / 可能的扩展字段）
            double value = 0.0;
            if (!real64(data, pos, value)) return false;
            if (field == 2) quote.price = value;
            else if (field == 8) quote.changePercent = value;
            else if (field == 10) quote.high = value;
            else if (field == 11) quote.low = value;
            else if (field == 12) quote.change = value;
        } else if (wire == 0) {
            // varint：time/volume 为 sint64（zigzag 编码）
            quint64 value = 0;
            if (!varint(data, pos, value)) return false;
            if (field == 3) quote.timestamp = zigzag64(value);
            else if (field == 9) quote.volume = zigzag64(value);
        } else if (wire == 2) {
            QByteArray value;
            if (!bytes(data, pos, value)) return false;
            if (field == 1) quote.symbol = QString::fromUtf8(value);
            else if (field == 4) quote.currency = QString::fromUtf8(value);
            else if (field == 5) quote.exchange = QString::fromUtf8(value);
        } else if (wire == 5) {
            // 32-bit float：Yahoo 当前把 price/change/changePercent/high/low 用 float 发送
            float value = 0.0f;
            if (!real32(data, pos, value)) return false;
            if (field == 2) quote.price = static_cast<double>(value);
            else if (field == 8) quote.changePercent = static_cast<double>(value);
            else if (field == 10) quote.high = static_cast<double>(value);
            else if (field == 11) quote.low = static_cast<double>(value);
            else if (field == 12) quote.change = static_cast<double>(value);
        } else {
            return false;
        }
    }
    quote.prevClose = quote.price - quote.change;
    return quote.isValid();
}

QString YahooWebSocketAdapter::normalize(const QString& symbol) { return symbol.trimmed().toUpper(); }

} // namespace fininsight::datahub
