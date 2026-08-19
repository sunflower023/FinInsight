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
}

YahooWebSocketAdapter::YahooWebSocketAdapter(QObject* parent)
    : YahooWebSocketAdapter(kDefaultEndpoint, parent) {}

YahooWebSocketAdapter::YahooWebSocketAdapter(const QUrl& endpoint, QObject* parent)
    : QuoteStream(parent), endpoint_(endpoint), client_(new network::WebSocketClient(this)) {
    client_->setHeartbeatInterval(15000);
    client_->setReconnectPolicy(true, 500, 30000, -1);
    connect(client_, &network::WebSocketClient::connected, this, &YahooWebSocketAdapter::onConnected);
    connect(client_, &network::WebSocketClient::binaryMessageReceived, this, &YahooWebSocketAdapter::onBinaryMessage);
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
            double value = 0.0;
            if (!real64(data, pos, value)) return false;
            if (field == 2) quote.price = value;
            else if (field == 8) quote.changePercent = value;
            else if (field == 9) quote.volume = static_cast<qint64>(value);
            else if (field == 10) quote.high = value;
            else if (field == 11) quote.low = value;
            else if (field == 12) quote.change = value;
        } else if (wire == 0) {
            quint64 value = 0;
            if (!varint(data, pos, value)) return false;
            if (field == 3) quote.timestamp = static_cast<qint64>(value);
        } else if (wire == 2) {
            QByteArray value;
            if (!bytes(data, pos, value)) return false;
            if (field == 1) quote.symbol = QString::fromUtf8(value);
            else if (field == 4) quote.currency = QString::fromUtf8(value);
            else if (field == 5) quote.exchange = QString::fromUtf8(value);
        } else if (wire == 5) {
            if (pos + 4 > data.size()) return false;
            pos += 4;
        } else {
            return false;
        }
    }
    quote.prevClose = quote.price - quote.change;
    return quote.isValid();
}

QString YahooWebSocketAdapter::normalize(const QString& symbol) { return symbol.trimmed().toUpper(); }

} // namespace fininsight::datahub
