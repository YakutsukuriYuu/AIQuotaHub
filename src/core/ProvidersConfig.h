#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

// 单个提供商的运行配置（来自 providers.json）
struct ProviderConfig {
    QString id;                 // "glm"
    QString name;               // "GLM 智谱"
    QString type;               // "http"（默认）/ "demo"
    bool enabled = true;
    QString endpoint;
    QString authHeader;         // "Authorization"
    QString authPrefix;         // "Bearer "
    QString credentialKey;      // 钥匙串 account，如 "apiKey"
    int refreshIntervalSec = 120;
    bool supportsQuota = false;
    bool supportsApiUsage = false;
    QJsonObject fields;         // 字段路径覆盖（透传给解析器）
};

class ProvidersConfig
{
public:
    // 加载优先级：$AIQUOTAHUB_PROVIDERS_JSON
    //   > ~/Library/Application Support/AIQuotaHub/providers.json（用户覆盖）
    //   > App 包内 Resources/providers.json（随包默认）
    static QVector<ProviderConfig> load(QString *errorMessage = nullptr);
    static QString resolvedPath();

private:
    static ProviderConfig parseEntry(const QJsonObject &object);
};
