#include "CustomJsonParser.h"

#include "../core/JsonPath.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

ProviderSnapshot parseCustomJsonResponse(const QByteArray &body, const QJsonObject &fields,
                                         const QString &providerId)
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
    const QJsonValue root = doc.isObject() ? QJsonValue(doc.object())
                                           : QJsonValue(doc.array());

    bool anyData = false;

    // ---- 配额数组 ----
    const QString quotasPath = fields.value(QStringLiteral("quotasArray")).toString();
    if (!quotasPath.isEmpty()) {
        const QString usedKey = fields.value(QStringLiteral("used")).toString();
        const QString limitKey = fields.value(QStringLiteral("limit")).toString();
        const QString resetKey = fields.value(QStringLiteral("reset")).toString();
        const QString weeklyType = fields.value(QStringLiteral("weeklyType")).toString();
        const QString quotaLabel = fields.value(QStringLiteral("quotaLabel")).toString();

        const QJsonArray quotas = jsonValueAtPath(root, quotasPath).toArray();
        for (const QJsonValue &value : quotas) {
            const QJsonObject entry = value.toObject();

            QuotaWindow quota;
            const QString type = entry.value(QStringLiteral("type")).toString();
            const bool isWeekly = weeklyType.isEmpty()
                ? type.contains(QLatin1StringView("WEEK"), Qt::CaseInsensitive)
                : (type.compare(weeklyType, Qt::CaseInsensitive) == 0);
            quota.kind = isWeekly ? QuotaWindow::Kind::Weekly : QuotaWindow::Kind::FiveHour;
            quota.label = quotaLabel.isEmpty()
                ? (isWeekly ? QStringLiteral("周额度") : QStringLiteral("5小时额度"))
                : quotaLabel;

            quota.used = usedKey.isEmpty()
                ? jsonFirstOf(entry, {"currentValue", "usage", "used"}).toDouble(-1)
                : entry.value(usedKey).toDouble(-1);
            quota.limit = limitKey.isEmpty()
                ? jsonFirstOf(entry, {"limitValue", "total", "limit"}).toDouble(-1)
                : entry.value(limitKey).toDouble(-1);
            if (quota.used < 0 || quota.limit <= 0)
                continue;

            const QJsonValue resetValue = resetKey.isEmpty()
                ? jsonFirstOf(entry, {"nextResetTime", "resetTime", "resetAt"})
                : entry.value(resetKey);
            const double reset = resetValue.toDouble();
            if (reset > 1e12)
                quota.resetAt = QDateTime::fromMSecsSinceEpoch(qint64(reset));
            else if (reset > 1e9)
                quota.resetAt = QDateTime::fromSecsSinceEpoch(qint64(reset));

            snapshot.quotas.append(quota);
        }
        anyData = anyData || !snapshot.quotas.isEmpty();
    }

    // ---- 余额 ----
    const QString balancePath = fields.value(QStringLiteral("balance")).toString();
    if (!balancePath.isEmpty()) {
        const QJsonValue value = jsonValueAtPath(root, balancePath);

        bool ok = false;
        double balance = 0;
        if (value.isString())
            balance = value.toString().toDouble(&ok);
        else if (value.isDouble()) {
            balance = value.toDouble();
            ok = true;
        }

        if (ok) {
            ApiUsage api;
            api.label = fields.value(QStringLiteral("balanceLabel"))
                            .toString(QStringLiteral("余额"));
            api.balance = balance;
            api.isValid = true;

            QString currency = jsonValueAtPath(
                root, fields.value(QStringLiteral("currency")).toString()).toString();
            if (currency.isEmpty())
                currency = fields.value(QStringLiteral("currencyFixed"))
                               .toString(QStringLiteral("CNY"));
            api.currency = currency;

            snapshot.api = api;
            anyData = true;
        }
    }

    if (!anyData)
        snapshot.error = QStringLiteral("按自定义映射未解析到任何数据，请检查字段路径");

    return snapshot;
}
