#include "HttpJsonClient.h"

#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>

HttpJsonClient::HttpJsonClient(QObject *parent)
    : QObject(parent)
{
}

void HttpJsonClient::get(const QUrl &url,
                         const QString &authHeader,
                         const QString &authValue,
                         Callback callback,
                         int timeoutMs)
{
    QNetworkRequest request(url);
    request.setTransferTimeout(timeoutMs);
    request.setRawHeader("Accept", "application/json");
    if (!authHeader.isEmpty() && !authValue.isEmpty())
        request.setRawHeader(authHeader.toUtf8(), authValue.toUtf8());

    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this,
            [reply, callback = std::move(callback)]() mutable {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString message = reply->errorString();
            if (status > 0)
                message = QStringLiteral("HTTP %1：%2").arg(status).arg(message);
            callback({}, message);
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            callback({}, QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString()));
            return;
        }
        callback(doc, {});
    });
}
