#include "network/HttpClient.h"

#include <QNetworkRequest>
#include <QMetaObject>
#include <QThread>
#include <QUrl>

namespace fininsight::network {

bool HttpResponse::isSuccess() const {
    return !timedOut && !cancelled
        && networkError == QNetworkReply::NoError
        && statusCode >= 200 && statusCode < 300;
}

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

HttpClient::RequestId HttpClient::getAsync(const QString& url, QObject* context,
                                           ResponseHandler handler, int timeoutMs)
{
    Q_ASSERT_X(QThread::currentThread() == thread(), "HttpClient::getAsync",
               "getAsync must be called from the HttpClient thread");
    if (QThread::currentThread() != thread() || !context || !handler || timeoutMs <= 0) {
        return InvalidRequestId;
    }

    QNetworkRequest req{QUrl{url}};
    setupCommonHeaders(req);

    QNetworkReply* reply = mgr_->get(req);
    const RequestId requestId = nextRequestId_++;

    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);
    PendingRequest request;
    request.reply = reply;
    request.context = context;
    request.timer = timer;
    request.handler = std::move(handler);
    pending_.insert(requestId, std::move(request));

    connect(timer, &QTimer::timeout, this, [this, requestId]() {
        auto it = pending_.find(requestId);
        if (it == pending_.end() || !it->reply) return;
        it->timedOut = true;
        it->reply->abort();
    });
    timer->start(timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [this, requestId]() {
        finishRequest(requestId);
    });
    auto contextDestroyedConnection = connect(context, &QObject::destroyed, this, [this, requestId]() {
        cancel(requestId);
    });
    pending_[requestId].contextDestroyedConnection = contextDestroyedConnection;

    return requestId;
}

bool HttpClient::cancel(RequestId requestId)
{
    Q_ASSERT_X(QThread::currentThread() == thread(), "HttpClient::cancel",
               "cancel must be called from the HttpClient thread");
    if (QThread::currentThread() != thread()) return false;

    auto it = pending_.find(requestId);
    if (it == pending_.end() || !it->reply) return false;

    it->cancelled = true;
    it->reply->abort();
    return true;
}

void HttpClient::finishRequest(RequestId requestId)
{
    auto it = pending_.find(requestId);
    if (it == pending_.end()) return;

    PendingRequest request = std::move(it.value());
    pending_.erase(it);

    if (request.contextDestroyedConnection) {
        QObject::disconnect(request.contextDestroyedConnection);
    }
    if (request.timer) request.timer->stop();
    if (!request.reply) return;

    HttpResponse response;
    response.statusCode = request.reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.networkError = request.reply->error();
    response.timedOut = request.timedOut;
    response.cancelled = request.cancelled;
    if (request.reply->isOpen()) {
        response.body = request.reply->readAll();
    }

    if (response.timedOut) {
        response.error = "Request timed out";
    } else if (response.cancelled) {
        response.error = "Request cancelled";
    } else if (response.networkError != QNetworkReply::NoError) {
        response.error = request.reply->errorString();
    } else if (response.statusCode < 200 || response.statusCode >= 300) {
        response.error = QString("HTTP %1").arg(response.statusCode);
    }

    request.reply->deleteLater();
    if (!response.isSuccess()) lastError_ = response.error;

    if (request.context && request.handler) {
        QPointer<QObject> context = request.context;
        ResponseHandler handler = std::move(request.handler);
        QMetaObject::invokeMethod(context, [context, handler = std::move(handler), response]() {
            if (context) handler(response);
        }, Qt::QueuedConnection);
    }
}

} // namespace fininsight::network
