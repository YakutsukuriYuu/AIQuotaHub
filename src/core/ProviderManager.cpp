#include "ProviderManager.h"

#include "Provider.h"
#include "../providers/DemoProvider.h"
#include "../providers/HttpProvider.h"

#include <utility>

ProviderManager::ProviderManager(QObject *parent)
    : QObject(parent)
{
    reload();
}

void ProviderManager::reload()
{
    QString error;
    m_configs = ProvidersConfig::load(&error);
    if (!error.isEmpty())
        qWarning().noquote() << error;
    rebuild();
}

ProviderConfig ProviderManager::configFor(const QString &id) const
{
    for (const ProviderConfig &config : m_configs) {
        if (config.id == id)
            return config;
    }
    return {};
}

void ProviderManager::rebuild()
{
    qDeleteAll(m_providers);
    m_providers.clear();

    for (const ProviderConfig &config : std::as_const(m_configs)) {
        if (!config.enabled)
            continue;
        Provider *provider = nullptr;
        if (config.type == QStringLiteral("demo"))
            provider = new DemoProvider(config, this);
        else
            provider = new HttpProvider(config, this);
        m_providers.append(provider);
    }
    emit providersChanged();
}

void ProviderManager::upsertConfig(const ProviderConfig &config)
{
    bool found = false;
    for (ProviderConfig &existing : m_configs) {
        if (existing.id == config.id) {
            existing = config;
            found = true;
            break;
        }
    }
    if (!found)
        m_configs.append(config);

    persistUserLayer();
    rebuild();
}

void ProviderManager::removeConfig(const QString &id)
{
    for (int i = 0; i < m_configs.size(); ++i) {
        if (m_configs.at(i).id != id)
            continue;
        if (m_configs.at(i).source == QStringLiteral("user"))
            m_configs.removeAt(i);            // 用户源：物理删除
        else
            m_configs[i].enabled = false;     // 内置源：落禁用覆盖
        break;
    }
    persistUserLayer();
    rebuild();
}

void ProviderManager::persistUserLayer()
{
    // 用户层 = 全部 user 源 + 被禁用的 builtin 覆盖。
    // 内置源的其他字段不开放编辑（Key 在钥匙串，不落 JSON）。
    QVector<ProviderConfig> userLayer;
    for (const ProviderConfig &config : std::as_const(m_configs)) {
        if (config.source == QStringLiteral("user") || !config.enabled)
            userLayer.append(config);
    }

    QString error;
    if (!ProvidersConfig::saveUserConfigs(userLayer, &error))
        qWarning().noquote() << error;
}
