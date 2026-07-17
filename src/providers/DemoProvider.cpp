#include "DemoProvider.h"

#include <QRandomGenerator>
#include <QTimer>
#include <utility>

DemoProvider::DemoProvider(ProviderConfig config, QObject *parent)
    : Provider(parent)
    , m_config(std::move(config))
{
}

void DemoProvider::fetch()
{
    // 模拟网络延迟后返回一份随机但合理的快照
    QTimer::singleShot(350, this, [this] {
        auto *rng = QRandomGenerator::global();

        ProviderSnapshot snapshot;
        snapshot.providerId = m_config.id;
        snapshot.fetchedAt = QDateTime::currentDateTime();

        QuotaWindow fiveHour;
        fiveHour.label = QStringLiteral("5小时额度");
        fiveHour.used = rng->bounded(20, 96);
        fiveHour.limit = 100;
        fiveHour.resetAt = QDateTime::currentDateTime().addSecs(rng->bounded(600, 5 * 3600));

        QuotaWindow weekly;
        weekly.kind = QuotaWindow::Kind::Weekly;
        weekly.label = QStringLiteral("周额度");
        weekly.used = rng->bounded(60, 460);
        weekly.limit = 500;
        weekly.resetAt = QDateTime::currentDateTime().addSecs(rng->bounded(86400, 5 * 86400));

        snapshot.quotas = {fiveHour, weekly};

        ApiUsage api;
        api.currency = QStringLiteral("CNY");
        api.label = QStringLiteral("余额");
        api.balance = 42.50 + rng->bounded(1000) / 100.0;
        api.isValid = true;
        snapshot.api = api;

        emit finished(snapshot);
    });
}
