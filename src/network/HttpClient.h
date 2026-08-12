#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QString>
#include <QEventLoop>
#include <QTimer>
#include <QHash>
#include <QPointer>
#include <QMetaObject>

#include <functional>

class QNetworkRequest;

namespace fininsight::network {

/**
 * @brief HTTP 请求客户端（单例，基于 QNetworkAccessManager）
 *
 * 提供同步/异步 GET/POST：
 *   auto bytes = HttpClient::instance().get("https://...");
 *   HttpClient::instance().getAsync(url, this, [](const HttpResponse& r) { ... });
 */

struct HttpResponse {
    QByteArray body;
    int statusCode = 0;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
    QString error;
    bool timedOut = false;
    bool cancelled = false;

    bool isSuccess() const;
};

class HttpClient : public QObject {
    Q_OBJECT

public:
    using RequestId = quint64;
    using ResponseHandler = std::function<void(const HttpResponse&)>;
    static constexpr RequestId InvalidRequestId = 0;

    static HttpClient& instance();

    // —— 同步请求（阻塞当前线程，简单场景用） ——
    QByteArray get (const QString& url, int timeoutMs = 8000);
    QByteArray post(const QString& url, const QByteArray& body, int timeoutMs = 8000);

    // —— 异步请求（不阻塞；上下文销毁时自动取消回调） ——
    // 调用必须发生在 HttpClient 所在线程；context 不能为 nullptr。
    RequestId getAsync(const QString& url, QObject* context,
                       ResponseHandler handler, int timeoutMs = 8000);
    bool cancel(RequestId requestId);
    int activeRequestCount() const { return pending_.size(); }

    // —— 错误信息 ——
    QString lastError() const { return lastError_; }

private:
    explicit HttpClient(QObject* parent = nullptr);

    void setupCommonHeaders(QNetworkRequest& req);
    struct PendingRequest {
        QPointer<QNetworkReply> reply;
        QPointer<QObject> context;
        QTimer* timer = nullptr;
        ResponseHandler handler;
        QMetaObject::Connection contextDestroyedConnection;
        bool timedOut = false;
        bool cancelled = false;
    };

    void finishRequest(RequestId requestId);

    QNetworkAccessManager* mgr_;
    QString lastError_;
    QHash<RequestId, PendingRequest> pending_;
    RequestId nextRequestId_ = 1;
};

} // namespace fininsight::network
