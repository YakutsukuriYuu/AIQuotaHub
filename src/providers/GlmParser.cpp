#include "GlmParser.h"

#include "../core/JsonPath.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

ProviderSnapshot parseGlmQuotaResponse(const QByteArray &body, const QJsonObject &fields)
{
    ProviderSnapshot snapshot;
    snapshot.providerId = QStringLiteral("glm");
    snapshot.fetchedAt = QDateTime::currentDateTime();

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        snapshot.error = QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString());
        return snapshot;
    }

    const QJsonObject root = doc.object();

    // 智谱业务错误码：code 非 200 视为失败
    const int code = root.value(QStringLiteral("code")).toInt(200);
    if (code != 200) {
        snapshot.error = QStringLiteral("接口返回 code=%1：%2")
                             .arg(code)
                             .arg(root.value(QStringLiteral("msg")).toString());
        return snapshot;
    }

    const QString limitsPath = fields.value(QStringLiteral("limitsArray"))
                                   .toString(QStringLiteral("data.limits"));
    const QJsonArray limits = jsonValueAtPath(root, limitsPath).toArray();
    if (limits.isEmpty()) {
        snapshot.error = QStringLiteral("响应中没有配额数组（%1）").arg(limitsPath);
        return snapshot;
    }

    for (const QJsonValue &value : limits) {
        const QJsonObject entry = value.toObject();
        const QString type = entry.value(QStringLiteral("type")).toString();

        QuotaWindow quota;
        if (type.contains(QLatin1StringView("WEEK"), Qt::CaseInsensitive)) {
            quota.kind = QuotaWindow::Kind::Weekly;
            quota.label = QStringLiteral("周额度");
        } else {
            quota.kind = QuotaWindow::Kind::FiveHour;
            quota.label = QStringLiteral("5小时额度");
        }

        // 字段名做候选容忍：接口微调时不至于整单失败
        quota.used = jsonFirstOf(entry, {"currentValue", "usage", "used"}).toDouble(-1);
        quota.limit = jsonFirstOf(entry, {"limitValue", "total", "limit"}).toDouble(-1);
        if (quota.used < 0 || quota.limit <= 0)
            continue;

        const double reset = jsonFirstOf(entry, {"nextResetTime", "resetTime", "resetAt"})
                                 .toDouble();
        if (reset > 1e12)
            quota.resetAt = QDateTime::fromMSecsSinceEpoch(qint64(reset));
        else if (reset > 1e9)
            quota.resetAt = QDateTime::fromSecsSinceEpoch(qint64(reset));

        snapshot.quotas.append(quota);
    }

    if (snapshot.quotas.isEmpty())
        snapshot.error = QStringLiteral(
            "配额字段解析失败（接口结构可能已变化，请检查 providers.json 字段映射）");

    return snapshot;
}
