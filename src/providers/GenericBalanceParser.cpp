#include "GenericBalanceParser.h"

#include "../core/JsonPath.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

ProviderSnapshot parseGenericBalance(const QByteArray &body, const QJsonObject &fields,
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
    const QJsonObject root = doc.object();

    // ---- 形态 A：DeepSeek balance_infos ----
    const QJsonArray infos = root.value(QStringLiteral("balance_infos")).toArray();
    if (!infos.isEmpty()) {
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

    // ---- 形态 B：Moonshot data.available_balance ----
    // 业务错误先看 code：错误响应可能没有 data（{"code":5,"message":…}）
    const QJsonValue codeValue = root.value(QStringLiteral("code"));
    if (codeValue.isDouble() && codeValue.toInt() != 0) {
        snapshot.error = QStringLiteral("接口返回 code=%1：%2")
                             .arg(codeValue.toInt())
                             .arg(root.value(QStringLiteral("message")).toString());
        return snapshot;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    if (data.contains(QStringLiteral("available_balance"))) {
        ApiUsage api;
        api.currency = QStringLiteral("CNY");
        api.label = QStringLiteral("余额");
        api.balance = data.value(QStringLiteral("available_balance")).toDouble(-1);
        api.isValid = api.balance >= 0;
        if (!api.isValid) {
            snapshot.error = QStringLiteral("available_balance 字段解析失败");
            return snapshot;
        }
        const double voucher =
            data.value(QStringLiteral("voucher_balance")).toDouble();
        if (voucher > 0)
            api.note = QStringLiteral("含代金券 %1").arg(voucher);

        snapshot.api = api;
        return snapshot;
    }

    // ---- 形态 C：自定义路径 ----
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
            api.label = QStringLiteral("余额");
            api.balance = balance;
            api.isValid = true;
            QString currency = jsonValueAtPath(
                root, fields.value(QStringLiteral("currency")).toString()).toString();
            api.currency = currency.isEmpty() ? QStringLiteral("CNY") : currency;

            snapshot.api = api;
            return snapshot;
        }
    }

    // ---- 错误形态：{"error": {"message": …}} ----
    const QJsonObject errorObj = root.value(QStringLiteral("error")).toObject();
    if (!errorObj.isEmpty()) {
        snapshot.error = errorObj.value(QStringLiteral("message")).toString(
            QStringLiteral("接口返回错误"));
        return snapshot;
    }

    snapshot.error = QStringLiteral(
        "无法识别的余额响应结构（可换 custom_json 模板手动映射字段）");
    return snapshot;
}
