#include "CredentialStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

// 单机开源软件：凭据直接存本地 JSON 文件（权限 600 仅本人可读写）。
// 不用 macOS 钥匙串——开发版每次重编译签名变化都会触发系统授权弹窗。

namespace {

QString storePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/credentials.json");
}

QJsonObject loadStore()
{
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool saveStore(const QJsonObject &store, QString *errorMessage)
{
    const QString path = storePath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法创建目录：%1")
                                .arg(QFileInfo(path).absolutePath());
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法写入 %1").arg(path);
        return false;
    }
    file.write(QJsonDocument(store).toJson(QJsonDocument::Indented));
    file.close();

    // 仅本人可读写
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

} // namespace

bool CredentialStore::save(const QString &service, const QString &account,
                           const QString &secret, QString *errorMessage)
{
    QJsonObject store = loadStore();
    QJsonObject serviceEntry = store.value(service).toObject();
    serviceEntry.insert(account, secret);
    store.insert(service, serviceEntry);
    return saveStore(store, errorMessage);
}

QString CredentialStore::read(const QString &service, const QString &account)
{
    return loadStore().value(service).toObject().value(account).toString();
}

bool CredentialStore::remove(const QString &service, const QString &account)
{
    QJsonObject store = loadStore();
    QJsonObject serviceEntry = store.value(service).toObject();
    if (!serviceEntry.contains(account))
        return false;

    serviceEntry.remove(account);
    if (serviceEntry.isEmpty())
        store.remove(service);
    else
        store.insert(service, serviceEntry);
    return saveStore(store, nullptr);
}
