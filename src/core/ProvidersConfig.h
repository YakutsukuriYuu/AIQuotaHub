#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

// 单个提供商的运行配置（来自 providers.json）
struct ProviderConfig {
    QString id;                 // "glm"；用户自建源用 "user-xxx"
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
    QString parserTemplate;     // glm_quota / deepseek_balance / moonshot_balance
                                // / openai_costs / custom_json
    QString source;             // "builtin"（随包）/ "user"（用户自建）
    QJsonObject fields;         // 字段路径覆盖（透传给解析器）
};

class ProvidersConfig
{
public:
    // 合并加载：内置（App 包 Resources）为基底，用户层文件按 id 覆盖/追加。
    // $AIQUOTAHUB_PROVIDERS_JSON 存在时整体替换（调试用）。
    static QVector<ProviderConfig> load(QString *errorMessage = nullptr);

    // 用户层文件（可写）：~/Library/Application Support/AIQuotaHub/providers.json
    static QString userFilePath();
    static bool saveUserConfigs(const QVector<ProviderConfig> &configs,
                                QString *errorMessage = nullptr);

    static QJsonObject entryToJson(const ProviderConfig &config);
    static ProviderConfig parseEntry(const QJsonObject &object);
};
