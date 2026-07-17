#include "ProvidersConfig.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>

namespace {

QStringList candidatePaths()
{
    QStringList paths;

    const QByteArray env = qgetenv("AIQUOTAHUB_PROVIDERS_JSON");
    if (!env.isEmpty())
        paths.append(QString::fromUtf8(env));

    // macOS 上即 ~/Library/Application Support/AIQuotaHub
    const QString supportDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!supportDir.isEmpty())
        paths.append(supportDir + QStringLiteral("/providers.json"));

    // macOS App 包：Contents/MacOS/../Resources/providers.json
    paths.append(QCoreApplication::applicationDirPath()
                 + QStringLiteral("/../Resources/providers.json"));

    return paths;
}

} // namespace

QString ProvidersConfig::resolvedPath()
{
    const QStringList paths = candidatePaths();
    for (const QString &path : paths) {
        if (QFile::exists(path))
            return path;
    }
    return paths.isEmpty() ? QString() : paths.first();
}

QVector<ProviderConfig> ProvidersConfig::load(QString *errorMessage)
{
    const QString path = resolvedPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("找不到 providers.json（%1）").arg(path);
        return {};
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage)
            *errorMessage = QStringLiteral("providers.json 解析失败：%1").arg(parseError.errorString());
        return {};
    }

    QVector<ProviderConfig> configs;
    const QJsonArray entries = doc.object().value(QStringLiteral("providers")).toArray();
    for (const QJsonValue &value : entries)
        configs.append(parseEntry(value.toObject()));
    return configs;
}

ProviderConfig ProvidersConfig::parseEntry(const QJsonObject &object)
{
    ProviderConfig config;
    config.id = object.value(QStringLiteral("id")).toString();
    config.name = object.value(QStringLiteral("name")).toString(config.id);
    config.type = object.value(QStringLiteral("type")).toString(QStringLiteral("http"));
    config.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    config.endpoint = object.value(QStringLiteral("endpoint")).toString();
    config.authHeader = object.value(QStringLiteral("authHeader")).toString();
    config.authPrefix = object.value(QStringLiteral("authPrefix")).toString();
    config.credentialKey = object.value(QStringLiteral("credentialKey"))
                               .toString(QStringLiteral("apiKey"));
    config.refreshIntervalSec =
        object.value(QStringLiteral("refreshIntervalSec")).toInt(120);
    config.supportsQuota = object.value(QStringLiteral("supportsQuota")).toBool(false);
    config.supportsApiUsage = object.value(QStringLiteral("supportsApiUsage")).toBool(false);
    config.fields = object.value(QStringLiteral("fields")).toObject();
    return config;
}
