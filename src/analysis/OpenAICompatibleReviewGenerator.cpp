#include "analysis/OpenAICompatibleReviewGenerator.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
namespace fininsight::analysis {
OpenAICompatibleReviewGenerator::OpenAICompatibleReviewGenerator(Config config, QObject* parent) : QObject(parent), config_(std::move(config)) {}
bool OpenAICompatibleReviewGenerator::isConfigured() const { return !config_.endpoint.trimmed().isEmpty() && !config_.apiKey.isEmpty() && !config_.model.trimmed().isEmpty(); }
QString OpenAICompatibleReviewGenerator::generateAsync(const EvidenceSnapshot& evidence, const BehaviorReport& report)
{
    cancel(); requestId_ = QUuid::createUuid().toString(QUuid::WithoutBraces); const QString id = requestId_;
    if (!isConfigured()) { QMetaObject::invokeMethod(this, [this,id]{ emit failed(id, QStringLiteral("Model provider is not configured")); }, Qt::QueuedConnection); return id; }
    QMap<QByteArray,QByteArray> headers; headers.insert("Authorization", "Bearer " + config_.apiKey.toUtf8());
    networkRequestId_ = network::HttpClient::instance().postAsync(config_.endpoint, buildRequest(evidence, report), this,
        [this,id](const network::HttpResponse& response) {
            if (id != requestId_) return; networkRequestId_ = network::HttpClient::InvalidRequestId;
            if (!response.isSuccess()) { emit failed(id, response.error.isEmpty() ? QStringLiteral("Model request failed") : response.error); return; }
            QJsonParseError parseError; const auto doc = QJsonDocument::fromJson(response.body, &parseError);
            const auto choices = doc.object().value(QStringLiteral("choices")).toArray();
            const QString content = choices.isEmpty() ? QString() : choices.first().toObject().value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString().trimmed();
            if (parseError.error != QJsonParseError::NoError || content.isEmpty()) { emit failed(id, QStringLiteral("Model returned an invalid response")); return; }
            emit completed(id, content);
        }, config_.timeoutMs, headers);
    if (networkRequestId_ == network::HttpClient::InvalidRequestId) QMetaObject::invokeMethod(this, [this,id]{ emit failed(id, QStringLiteral("Model request could not start")); }, Qt::QueuedConnection);
    return id;
}
bool OpenAICompatibleReviewGenerator::cancel()
{
    requestId_.clear(); if (networkRequestId_ == network::HttpClient::InvalidRequestId) return false;
    const bool result = network::HttpClient::instance().cancel(networkRequestId_); networkRequestId_ = network::HttpClient::InvalidRequestId; return result;
}
QByteArray OpenAICompatibleReviewGenerator::buildRequest(const EvidenceSnapshot& evidence, const BehaviorReport& report) const
{
    QJsonArray findings; for (const auto& f : report.findings) { QJsonArray ids; for (auto id : f.evidenceTradeIds) ids.append(double(id));
        findings.append(QJsonObject{{"code",QString::fromStdString(f.code)},{"message",QString::fromStdString(f.message)},{"measure",f.measure},{"trade_ids",ids}}); }
    QJsonObject evidenceObject{{"trade_count",int(evidence.trades.size())},{"return_rate",evidence.portfolio.returnRate},{"max_drawdown",evidence.maxDrawdown},{"findings",findings}};
    const QString system = QStringLiteral("You review simulated trading evidence. Separate facts, inferences, educational notes and risks. Cite finding codes or trade IDs. Never promise returns, recommend securities, or issue trading instructions. Return concise plain text.");
    QJsonArray messages{QJsonObject{{"role","system"},{"content",system}}, QJsonObject{{"role","user"},{"content",QString::fromUtf8(QJsonDocument(evidenceObject).toJson(QJsonDocument::Compact))}}};
    return QJsonDocument(QJsonObject{{"model",config_.model},{"messages",messages},{"temperature",0.2},{"max_tokens",config_.maxTokens}}).toJson(QJsonDocument::Compact);
}
} // namespace fininsight::analysis
