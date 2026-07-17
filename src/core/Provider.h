#pragma once

#include "Models.h"

#include <QObject>

// 提供商抽象接口：每个数据源一个子类，异步抓取后发射 finished()
class Provider : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~Provider() override = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool needsCredential() const { return true; }
    virtual int refreshIntervalSec() const { return 120; }

    // 异步抓取一次；完成后保证恰好发射一次 finished()
    virtual void fetch() = 0;

signals:
    void finished(const ProviderSnapshot &snapshot);
};
