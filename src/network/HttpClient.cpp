#include "network/HttpClient.h"

#include <QNetworkRequest>
#include <QUrl>

namespace fininsight::network {

// ── 单例 ────────────────────────────────────────────

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , mgr_(new QNetworkAccessManager(this))
{}

HttpClient& HttpClient::instance() {
    static HttpClient client;
    return client;
}

// ── 通用头部 ────────────────────────────────────────

void HttpClient::setupCommonHeaders(QNetworkRequest& req) {
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    req.setRawHeader("Accept", "application/json");
}

// ── 同步 GET ────────────────────────────────────────

QByteArray HttpClient::get(const QString& url, int timeoutMs) {
    QNetworkRequest req{QUrl{url}};
    setupCommonHeaders(req);

    QNetworkReply* reply = mgr_->get(req);

    // 用 QEventLoop 把异步转同步
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);

    timer.start(timeoutMs);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        lastError_ = reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

// ── 同步 POST ───────────────────────────────────────

QByteArray HttpClient::post(const QString& url, const QByteArray& body, int timeoutMs) {
    QNetworkRequest req{QUrl{url}};
    setupCommonHeaders(req);
    req.setRawHeader("Content-Type", "application/json");

    QNetworkReply* reply = mgr_->post(req, body);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);

    timer.start(timeoutMs);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        lastError_ = reply->errorString();
        reply->deleteLater();
        return {};
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

// ── 异步 GET ────────────────────────────────────────

void HttpClient::getAsync(const QString& url,
                           std::function<void(QByteArray)> onDone,
                           std::function<void(QString)> onError,
                           int timeoutMs)
{
    QNetworkRequest req{QUrl{url}};
    setupCommonHeaders(req);

    QNetworkReply* reply = mgr_->get(req);

    // 超时定时器
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply, onError]() {
        reply->abort();
        if (onError) onError("Request timeout");
    });
    timer->start(timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [this, reply, onDone, onError]() {
        handleReply(reply, onDone, onError);
    });
}

void HttpClient::handleReply(QNetworkReply* reply,
                              std::function<void(QByteArray)> onDone,
                              std::function<void(QString)> onError)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        lastError_ = reply->errorString();
        if (onError) onError(lastError_);
        return;
    }

    if (onDone) onDone(reply->readAll());
}

} // namespace fininsight::network
