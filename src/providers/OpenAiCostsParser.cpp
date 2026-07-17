#include "OpenAiCostsParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ProviderSnapshot parseOpenAiCosts(const QByteArray &body, const QString &providerId)
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

    // OpenAI 错误形态：{"error": {"message": "…", "type": "…"}}
    const QJsonObject errorObj = root.value(QStringLiteral("error")).toObject();
    if (!errorObj.isEmpty()) {
        snapshot.error = errorObj.value(QStringLiteral("message")).toString(
            QStringLiteral("接口返回错误"));
        return snapshot;
    }

    const QJsonArray buckets = root.value(QStringLiteral("data")).toArray();
    if (buckets.isEmpty()) {
        snapshot.error = QStringLiteral("响应中没有计费数据（data 为空）");
        return snapshot;
    }

    double total = 0;
    QString currency;
    bool found = false;
    for (const QJsonValue &bucketValue : buckets) {
        const QJsonArray results = bucketValue.toObject()
                                       .value(QStringLiteral("results")).toArray();
        for (const QJsonValue &resultValue : results) {
            const QJsonObject amount = resultValue.toObject()
                                           .value(QStringLiteral("amount")).toObject();
            total += amount.value(QStringLiteral("value")).toDouble();
            if (currency.isEmpty())
                currency = amount.value(QStringLiteral("currency")).toString();
            found = true;
        }
    }

    if (!found) {
        snapshot.error = QStringLiteral("计费数据中没有 amount 字段");
        return snapshot;
    }

    ApiUsage api;
    api.currency = currency.isEmpty() ? QStringLiteral("USD") : currency.toUpper();
    api.label = QStringLiteral("本月花费");
    api.balance = total;
    api.isValid = true;
    if (root.value(QStringLiteral("has_more")).toBool())
        api.note = QStringLiteral("数据跨页，金额为部分汇总");

    snapshot.api = api;
    return snapshot;
}
