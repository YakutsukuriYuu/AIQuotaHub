#include "ProvidersConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStandardPaths>

namespace {

QString bundledFilePath()
{
    return QCoreApplication::applicationDirPath()
           + QStringLiteral("/../Resources/providers.json");
}

QVector<ProviderConfig> parseFile(const QString &path, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法读取 %1").arg(path);
        return {};
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage)
            *errorMessage = QStringLiteral("%1 解析失败：%2").arg(path, parseError.errorString());
        return {};
    }

    QVector<ProviderConfig> configs;
    const QJsonArray entries = doc.object().value(QStringLiteral("providers")).toArray();
    for (const QJsonValue &value : entries)
        configs.append(ProvidersConfig::parseEntry(value.toObject()));
    return configs;
}

} // namespace

QString ProvidersConfig::userFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/providers.json");
}

QVector<ProviderConfig> ProvidersConfig::load(QString *errorMessage)
{
    // 调试：环境变量整体替换
    const QByteArray env = qgetenv("AIQUOTAHUB_PROVIDERS_JSON");
    if (!env.isEmpty() && QFile::exists(QString::fromUtf8(env)))
        return parseFile(QString::fromUtf8(env), errorMessage);

    QVector<ProviderConfig> configs = parseFile(bundledFilePath(), errorMessage);

    // 用户层：同 id 覆盖内置，新 id 追加并标记 user
    const QString userPath = userFilePath();
    if (QFile::exists(userPath)) {
        QString userError;
        const QVector<ProviderConfig> userConfigs = parseFile(userPath, &userError);
        if (!userError.isEmpty())
            qWarning().noquote() << userError;

        for (ProviderConfig userConfig : userConfigs) {
            bool merged = false;
            for (ProviderConfig &existing : configs) {
                if (existing.id == userConfig.id) {
                    userConfig.source = existing.source;   // 覆盖不改内置身份
                    existing = userConfig;
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                userConfig.source = QStringLiteral("user");
                configs.append(userConfig);
            }
        }
    }
    return configs;
}

bool ProvidersConfig::saveUserConfigs(const QVector<ProviderConfig> &configs,
                                      QString *errorMessage)
{
    const QString path = userFilePath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法创建目录：%1").arg(QFileInfo(path).absolutePath());
        return false;
    }

    QJsonArray array;
    for (const ProviderConfig &config : configs)
        array.append(entryToJson(config));

    QJsonObject root;
    root.insert(QStringLiteral("providers"), array);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法写入 %1").arg(path);
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject ProvidersConfig::entryToJson(const ProviderConfig &config)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), config.id);
    object.insert(QStringLiteral("name"), config.name);
    object.insert(QStringLiteral("type"), config.type);
    object.insert(QStringLiteral("enabled"), config.enabled);
    object.insert(QStringLiteral("endpoint"), config.endpoint);
    object.insert(QStringLiteral("authHeader"), config.authHeader);
    object.insert(QStringLiteral("authPrefix"), config.authPrefix);
    object.insert(QStringLiteral("credentialKey"), config.credentialKey);
    object.insert(QStringLiteral("refreshIntervalSec"), config.refreshIntervalSec);
    object.insert(QStringLiteral("supportsQuota"), config.supportsQuota);
    object.insert(QStringLiteral("supportsApiUsage"), config.supportsApiUsage);
    object.insert(QStringLiteral("parserTemplate"), config.parserTemplate);
    if (!config.fields.isEmpty())
        object.insert(QStringLiteral("fields"), config.fields);
    return object;
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
    config.parserTemplate = object.value(QStringLiteral("parserTemplate")).toString();
    config.source = object.value(QStringLiteral("source"))
                        .toString(QStringLiteral("builtin"));
    config.fields = object.value(QStringLiteral("fields")).toObject();
    return config;
}
