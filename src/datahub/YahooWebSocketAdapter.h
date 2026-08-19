#pragma once

#include "datahub/QuoteStream.h"

#include <QObject>
#include <QSet>
#include <QUrl>

class QJsonObject;

namespace fininsight::network { class WebSocketClient; }

namespace fininsight::datahub {

/** Experimental, unauthenticated Yahoo Finance streamer adapter. */
class YahooWebSocketAdapter final : public QuoteStream {
    Q_OBJECT
public:
    explicit YahooWebSocketAdapter(QObject* parent = nullptr);
    explicit YahooWebSocketAdapter(const QUrl& endpoint, QObject* parent = nullptr);
    void start() override;
    void stop() override;
    void subscribe(const QString& symbol) override;
    void unsubscribe(const QString& symbol) override;
    bool isConnected() const override;

private slots:
    void onConnected();
    void onBinaryMessage(const QByteArray& message);
    void onError(const QString& error);

private:
    void sendSymbols(const QString& action, const QStringList& symbols);
    static bool parsePricingData(const QByteArray& bytes, QuoteData& quote);
    static QString normalize(const QString& symbol);

    QUrl endpoint_;
    network::WebSocketClient* client_;
    QSet<QString> symbols_;
};

} // namespace fininsight::datahub
