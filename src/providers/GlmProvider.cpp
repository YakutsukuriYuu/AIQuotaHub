#include "GlmProvider.h"

#include "../core/CredentialStore.h"
#include "../core/HttpJsonClient.h"
#include "GlmParser.h"

#include <utility>

GlmProvider::GlmProvider(ProviderConfig config, QObject *parent)
    : Provider(parent)
    , m_config(std::move(config))
    , m_http(new HttpJsonClient(this))
{
}

void GlmProvider::fetch()
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

    m_http->get(QUrl(m_config.endpoint), m_config.authHeader, m_config.authPrefix + apiKey,
                [this](const QJsonDocument &doc, const QString &error) {
        if (!error.isEmpty()) {
            ProviderSnapshot snapshot;
            snapshot.providerId = m_config.id;
            snapshot.fetchedAt = QDateTime::currentDateTime();
            snapshot.error = error;
            emit finished(snapshot);
            return;
        }
        emit finished(parseGlmQuotaResponse(doc.toJson(), m_config.fields));
    });
}
