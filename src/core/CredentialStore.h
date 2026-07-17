#pragma once

#include <QString>

// macOS 钥匙串封装：按 (service, account) 存取一条密码
class CredentialStore
{
public:
    static bool save(const QString &service, const QString &account,
                     const QString &secret, QString *errorMessage = nullptr);
    static QString read(const QString &service, const QString &account);
    static bool remove(const QString &service, const QString &account);
};
