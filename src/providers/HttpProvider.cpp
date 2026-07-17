#include "HttpProvider.h"

#include "../core/CredentialStore.h"
#include "../core/HttpJsonClient.h"
#include "CustomJsonParser.h"
#include "GenericBalanceParser.h"
#include "GlmParser.h"
#include "OpenAiCostsParser.h"

#include <QDate>
#include <utility>

HttpProvider::HttpProvider(ProviderConfig config, QObject *parent)
    : Provider(parent)
    , m_config(std::move(config))
    , m_http(new HttpJsonClient(this))
{
}

QVector<QPair<QString, QString>> HttpProvider::availableTemplates()
{
    // 按"格式大类"呈现，不提具体厂商：同类格式的服务都能接入
    return {
        {QStringLiteral("quota_windows"),
         QStringLiteral("订阅配额型（5 小时 / 周额度窗口）")},
        {QStringLiteral("balance_json"),
         QStringLiteral("账户余额型（通用 JSON，自动识别常见结构）")},
        {QStringLiteral("openai_costs"),
         QStringLiteral("用量费用型（OpenAI 兼容格式）")},
        {QStringLiteral("custom_json"),
         QStringLiteral("自定义字段映射（高级）")},
    };
}

QString HttpProvider::templateDisplayName(const QString &parserTemplate)
{
    const auto templates = availableTemplates();
    for (const auto &pair : templates) {
        if (pair.first == parserTemplate)
            return pair.second;
    }
    return parserTemplate.isEmpty() ? QStringLiteral("未指定") : parserTemplate;
}

QString HttpProvider::resolvedEndpoint() const
{
    QString endpoint = m_config.endpoint;
    if (endpoint.contains(QStringLiteral("{month_start}"))) {
        const QDate today = QDate::currentDate();
        // 默认构造即本地时区，无需显式 QTimeZone
        const QDateTime firstOfMonth(QDate(today.year(), today.month(), 1), QTime(0, 0));
        endpoint.replace(QStringLiteral("{month_start}"),
                         QString::number(firstOfMonth.toSecsSinceEpoch()));
    }
    return endpoint;
}

ProviderSnapshot HttpProvider::parse(const QByteArray &body) const
{
    // 新模板 id + 旧 id 别名（兼容已保存的用户配置）
    const QString &t = m_config.parserTemplate;
    if (t == QStringLiteral("quota_windows") || t == QStringLiteral("glm_quota"))
        return parseGlmQuotaResponse(body, m_config.fields, m_config.id);
    if (t == QStringLiteral("balance_json") || t == QStringLiteral("deepseek_balance")
        || t == QStringLiteral("moonshot_balance"))
        return parseGenericBalance(body, m_config.fields, m_config.id);
    if (t == QStringLiteral("openai_costs"))
        return parseOpenAiCosts(body, m_config.id);
    if (t == QStringLiteral("custom_json"))
        return parseCustomJsonResponse(body, m_config.fields, m_config.id);

    ProviderSnapshot snapshot;
    snapshot.providerId = m_config.id;
    snapshot.fetchedAt = QDateTime::currentDateTime();
    snapshot.error = QStringLiteral("未知解析模板：%1").arg(t);
    return snapshot;
}

void HttpProvider::fetch()
{
    const QString apiKey = CredentialStore::read(credentialServiceFor(m_config.id),
                                                 m_config.credentialKey);
    if (apiKey.isEmpty()) {
        ProviderSnapshot snapshot;
        snapshot.providerId = m_config.id;
        snapshot.fetchedAt = QDateTime::currentDateTime();
        snapshot.error = QStringLiteral("未配置 API Key（在设置页填写）");
        emit finished(snapshot);
        return;
    }

    m_http->get(QUrl(resolvedEndpoint()), m_config.authHeader,
                m_config.authPrefix + apiKey,
                [this](const QJsonDocument &doc, const QString &error) {
        if (!error.isEmpty()) {
            ProviderSnapshot snapshot;
            snapshot.providerId = m_config.id;
            snapshot.fetchedAt = QDateTime::currentDateTime();
            snapshot.error = error;
            emit finished(snapshot);
            return;
        }
        emit finished(parse(doc.toJson()));
    });
}
