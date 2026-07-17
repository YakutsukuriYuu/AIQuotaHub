#include "DeepSeekParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ProviderSnapshot parseDeepSeekBalance(const QByteArray &body)
{
    ProviderSnapshot snapshot;
    snapshot.providerId = QStringLiteral("deepseek");
    snapshot.fetchedAt = QDateTime::currentDateTime();

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        snapshot.error = QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString());
        return snapshot;
    }

    const QJsonObject root = doc.object();
    const QJsonArray infos = root.value(QStringLiteral("balance_infos")).toArray();
    if (infos.isEmpty()) {
        snapshot.error = QStringLiteral("响应中没有 balance_infos");
        return snapshot;
    }

    const QJsonObject info = infos.first().toObject();

    ApiUsage api;
    api.currency = info.value(QStringLiteral("currency")).toString(QStringLiteral("CNY"));
    api.label = QStringLiteral("余额");

    bool ok = false;
    api.balance = info.value(QStringLiteral("total_balance")).toString().toDouble(&ok);
    api.isValid = ok;
    if (!ok) {
        snapshot.error = QStringLiteral("total_balance 字段解析失败");
        return snapshot;
    }

    // 附加信息：赠送余额、余额不足标记
    QStringList notes;
    const double granted =
        info.value(QStringLiteral("granted_balance")).toString().toDouble();
    if (granted > 0)
        notes.append(QStringLiteral("含赠送 %1").arg(granted));
    if (!root.value(QStringLiteral("is_available")).toBool(true))
        notes.append(QStringLiteral("余额不足"));
    api.note = notes.join(QStringLiteral("；"));

    snapshot.api = api;
    return snapshot;
}
