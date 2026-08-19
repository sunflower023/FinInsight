#pragma once
#include "analysis/ReviewGenerator.h"
#include "network/HttpClient.h"
#include <QObject>
namespace fininsight::analysis {
class OpenAICompatibleReviewGenerator final : public QObject {
    Q_OBJECT
public:
    struct Config { QString endpoint; QString apiKey; QString model; int timeoutMs = 30000; int maxTokens = 1200; };
    explicit OpenAICompatibleReviewGenerator(Config config, QObject* parent = nullptr);
    QString generateAsync(const EvidenceSnapshot& evidence, const BehaviorReport& report);
    bool cancel();
    bool isConfigured() const;
signals:
    void completed(const QString& requestId, const QString& reviewText);
    void failed(const QString& requestId, const QString& error);
private:
    QByteArray buildRequest(const EvidenceSnapshot& evidence, const BehaviorReport& report) const;
    Config config_; network::HttpClient::RequestId networkRequestId_ = network::HttpClient::InvalidRequestId; QString requestId_;
};
} // namespace fininsight::analysis
