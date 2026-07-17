#pragma once

#include "../core/Provider.h"
#include "../core/ProvidersConfig.h"

// 演示数据源：生成模拟快照，无 API Key 时也能看到完整界面效果
class DemoProvider : public Provider
{
    Q_OBJECT
public:
    explicit DemoProvider(ProviderConfig config, QObject *parent = nullptr);

    QString id() const override { return m_config.id; }
    QString displayName() const override { return m_config.name; }
    bool needsCredential() const override { return false; }
    int refreshIntervalSec() const override { return m_config.refreshIntervalSec; }

    void fetch() override;

private:
    ProviderConfig m_config;
};
