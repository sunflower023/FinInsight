#pragma once

#include "datahub/QuoteData.h"
#include "network/HttpClient.h"

#include <QObject>
#include <QHash>
#include <QElapsedTimer>
#include <QSet>
#include <QTimer>
#include <QVector>

#include <functional>

namespace fininsight::datahub {

/**
 * @brief 多源行情异步聚合器。
 *
 * 同时请求可用数据源，选择第一个通过传输、载荷和领域校验的报价，
 * 然后取消其余请求。所有状态都运行在 Aggregator 所在线程。
 */
class Aggregator : public QObject {
    Q_OBJECT

public:
    using RequestStarter = std::function<network::HttpClient::RequestId(
        const QString&, QObject*, network::HttpClient::ResponseHandler, int)>;
    using RequestCanceller = std::function<bool(network::HttpClient::RequestId)>;

    explicit Aggregator(QObject* parent = nullptr);
    Aggregator(RequestStarter requestStarter,
               RequestCanceller requestCanceller,
               QObject* parent = nullptr);

    void fetchBest(const QString& symbol,
                   std::function<void(QuoteData)> onDone,
                   int timeoutMs = 5000);

    void fetchAll(const QString& symbol,
                  std::function<void(QuoteData, const QString& source)> onEach,
                  int timeoutMs = 5000);

    struct SourceResult {
        QString source;
        QString error;
        int latencyMs = 0;
        QuoteData data;
        bool valid = false;
    };

signals:
    void resultReady(const QuoteData& best, const QVector<SourceResult>& all);
    void errorOccurred(const QString& symbol, const QString& message);

private:
    struct AggregateState {
        quint64 id = 0;
        QString symbol;
        bool bestOnly = true;
        bool finished = false;
        QElapsedTimer elapsed;
        QSet<network::HttpClient::RequestId> requests;
        QSet<QString> pendingSources;
        QVector<SourceResult> results;
        std::function<void(QuoteData)> onDone;
        std::function<void(QuoteData, const QString&)> onEach;
        QTimer* timeoutTimer = nullptr;
    };

    using Parser = std::function<QuoteData(const QByteArray&, const QString&)>;

    quint64 createState(const QString& symbol, bool bestOnly, int timeoutMs);
    void startSource(quint64 stateId, const QString& source,
                     const QString& url, Parser parser);
    void handleSourceResponse(quint64 stateId, const QString& source,
                              Parser parser, const network::HttpResponse& response);
    void finishState(quint64 stateId, const QString& error = {});
    bool validateQuote(const QuoteData& quote, const QString& symbol) const;

    QHash<quint64, AggregateState> states_;
    quint64 nextStateId_ = 1;
    RequestStarter requestStarter_;
    RequestCanceller requestCanceller_;
};

} // namespace fininsight::datahub
