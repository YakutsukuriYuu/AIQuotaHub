#pragma once

#include "../core/Models.h"
#include "../core/Provider.h"
#include "../core/ProvidersConfig.h"

#include <QPair>
#include <QVector>

class HttpJsonClient;

// 通用 HTTP 提供商：按 parserTemplate 绑定解析函数。
// 任何"Bearer Key + JSON 响应"的数据源都能通过配置接入，无需写代码。
class HttpProvider : public Provider
{
    Q_OBJECT
public:
    explicit HttpProvider(ProviderConfig config, QObject *parent = nullptr);

    QString id() const override { return m_config.id; }
    QString displayName() const override { return m_config.name; }
    int refreshIntervalSec() const override { return m_config.refreshIntervalSec; }

    void fetch() override;

    // 模板注册表：(模板标识, 展示说明)，设置页下拉用
    static QVector<QPair<QString, QString>> availableTemplates();

private:
    QString resolvedEndpoint() const;              // 替换 {month_start} 等占位符
    ProviderSnapshot parse(const QByteArray &body) const;

    ProviderConfig m_config;
    HttpJsonClient *m_http;
};
