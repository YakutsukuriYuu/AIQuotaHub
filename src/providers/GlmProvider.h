#pragma once

#include "../core/Provider.h"
#include "../core/ProvidersConfig.h"

class HttpJsonClient;

class GlmProvider : public Provider
{
    Q_OBJECT
public:
    explicit GlmProvider(ProviderConfig config, QObject *parent = nullptr);

    QString id() const override { return m_config.id; }
    QString displayName() const override { return m_config.name; }
    int refreshIntervalSec() const override { return m_config.refreshIntervalSec; }

    void fetch() override;

private:
    ProviderConfig m_config;
    HttpJsonClient *m_http;
};
