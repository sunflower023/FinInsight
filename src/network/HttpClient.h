#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QString>
#include <QEventLoop>
#include <QTimer>

#include <functional>

namespace fininsight::network {

/**
 * @brief HTTP 请求客户端（单例，基于 QNetworkAccessManager）
 *
 * 提供同步/异步 GET/POST：
 *   auto bytes = HttpClient::instance().get("https://...");
 *   HttpClient::instance().getAsync("https://...", [](QByteArray r){ ... });
 */
class HttpClient : public QObject {
    Q_OBJECT

public:
    static HttpClient& instance();

    // —— 同步请求（阻塞当前线程，简单场景用） ——
    QByteArray get (const QString& url, int timeoutMs = 8000);
    QByteArray post(const QString& url, const QByteArray& body, int timeoutMs = 8000);

    // —— 异步请求（不阻塞，结果通过回调返回） ——
    void getAsync(const QString& url,
                  std::function<void(QByteArray)> onDone,
                  std::function<void(QString)> onError = nullptr,
                  int timeoutMs = 8000);

    // —— 错误信息 ——
    QString lastError() const { return lastError_; }

private:
    explicit HttpClient(QObject* parent = nullptr);

    void setupCommonHeaders(QNetworkRequest& req);
    void handleReply(QNetworkReply* reply,
                     std::function<void(QByteArray)> onDone,
                     std::function<void(QString)> onError);

    QNetworkAccessManager* mgr_;
    QString lastError_;
};

} // namespace fininsight::network
