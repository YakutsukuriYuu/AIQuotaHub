#pragma once

#include <QHash>
#include <QObject>
#include <QVector>

class Provider;

// 按各自间隔定时驱动 Provider::fetch()；失败时指数退避（×2，上限 15 分钟）
class RefreshScheduler : public QObject
{
    Q_OBJECT
public:
    explicit RefreshScheduler(QObject *parent = nullptr);

    void addProvider(Provider *provider);

public slots:
    void refreshAll();

private:
    QVector<Provider *> m_providers;
    QHash<Provider *, int> m_failures;   // 连续失败次数
};
