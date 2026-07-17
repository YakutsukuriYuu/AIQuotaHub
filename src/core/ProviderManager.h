#pragma once

#include "ProvidersConfig.h"

#include <QObject>
#include <QVector>

class Provider;

// 提供商管理器：持有生效配置与 Provider 实例。
// 新增/编辑/删除/启停 → 持久化用户层 JSON → 重建实例 → 发 providersChanged()
class ProviderManager : public QObject
{
    Q_OBJECT
public:
    explicit ProviderManager(QObject *parent = nullptr);

    const QVector<ProviderConfig> &configs() const { return m_configs; }
    const QVector<Provider *> &providers() const { return m_providers; }
    ProviderConfig configFor(const QString &id) const;

    void upsertConfig(const ProviderConfig &config);   // 新增或更新
    void removeConfig(const QString &id);              // user 源物理删除；builtin 落禁用覆盖
    void reload();                                     // 从磁盘重新加载

signals:
    void providersChanged();

private:
    void rebuild();
    void persistUserLayer();

    QVector<ProviderConfig> m_configs;
    QVector<Provider *> m_providers;
};
