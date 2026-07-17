#include "RefreshScheduler.h"

#include "Provider.h"

#include <QTimer>
#include <utility>

namespace {
constexpr int kMaxBackoffSec = 15 * 60;
}

RefreshScheduler::RefreshScheduler(QObject *parent)
    : QObject(parent)
{
}

void RefreshScheduler::addProvider(Provider *provider)
{
    m_providers.append(provider);
    m_failures.insert(provider, 0);

    connect(provider, &Provider::finished, this,
            [this, provider](const ProviderSnapshot &snapshot) {
        int delaySec = provider->refreshIntervalSec();
        if (snapshot.ok()) {
            m_failures.insert(provider, 0);
        } else {
            const int failures = ++m_failures[provider];
            delaySec = qMin(delaySec * (1 << qMin(failures, 4)), kMaxBackoffSec);
        }
        QTimer::singleShot(delaySec * 1000, provider, [provider] { provider->fetch(); });
    });

    connect(provider, &QObject::destroyed, this, [this, provider] {
        m_providers.removeAll(provider);
        m_failures.remove(provider);
    });
}

void RefreshScheduler::refreshAll()
{
    for (Provider *provider : std::as_const(m_providers))
        provider->fetch();
}
