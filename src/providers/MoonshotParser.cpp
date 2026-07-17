#include "MoonshotParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ProviderSnapshot parseMoonshotBalance(const QByteArray &body, const QString &providerId)
{
    ProviderSnapshot snapshot;
    snapshot.providerId = providerId;
    snapshot.fetchedAt = QDateTime::currentDateTime();

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        snapshot.error = QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString());
        return snapshot;
    }

    const QJsonObject root = doc.object();

    // 业务失败：{"code": 非0, "message": "…", "status": false}
    const int code = root.value(QStringLiteral("code")).toInt(0);
    if (code != 0) {
        snapshot.error = QStringLiteral("接口返回 code=%1：%2")
                             .arg(code)
                             .arg(root.value(QStringLiteral("message")).toString());
        return snapshot;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    if (data.isEmpty()) {
        snapshot.error = QStringLiteral("响应中没有 data 字段");
        return snapshot;
    }

    ApiUsage api;
    api.currency = QStringLiteral("CNY");
    api.label = QStringLiteral("余额");
    api.balance = data.value(QStringLiteral("available_balance")).toDouble(-1);
    api.isValid = api.balance >= 0;
    if (!api.isValid) {
        snapshot.error = QStringLiteral("available_balance 字段解析失败");
        return snapshot;
    }

    QStringList notes;
    const double voucher = data.value(QStringLiteral("voucher_balance")).toDouble();
    if (voucher > 0)
        notes.append(QStringLiteral("含代金券 %1").arg(voucher));
    api.note = notes.join(QStringLiteral("；"));

    snapshot.api = api;
    return snapshot;
}
