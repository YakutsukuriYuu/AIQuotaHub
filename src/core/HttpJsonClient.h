#pragma once

#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QUrl>
#include <functional>

// 极简异步 JSON GET 客户端：一次请求一个回调，错误统一成字符串
class HttpJsonClient : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void(const QJsonDocument &doc, const QString &error)>;

    explicit HttpJsonClient(QObject *parent = nullptr);

    // authHeader 为空则不带认证头；authValue 原样写入（调用方负责加 "Bearer " 前缀）
    void get(const QUrl &url,
             const QString &authHeader,
             const QString &authValue,
             Callback callback,
             int timeoutMs = 15000);

private:
    QNetworkAccessManager m_nam;
};
