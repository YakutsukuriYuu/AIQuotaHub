#include "CredentialStore.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

namespace {

NSMutableDictionary *baseQuery(const QString &service, const QString &account)
{
    return [@{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: service.toNSString(),
        (__bridge id)kSecAttrAccount: account.toNSString(),
    } mutableCopy];
}

QString statusError(OSStatus status, const char *action)
{
    const CFStringRef message = SecCopyErrorMessageString(status, nullptr);
    QString text = message ? QString::fromCFString(message)
                           : QStringLiteral("OSStatus %1").arg(status);
    if (message)
        CFRelease(message);
    return QStringLiteral("%1失败：%2").arg(QLatin1StringView(action), text);
}

} // namespace

bool CredentialStore::save(const QString &service, const QString &account,
                           const QString &secret, QString *errorMessage)
{
    NSData *data = [secret.toNSString() dataUsingEncoding:NSUTF8StringEncoding];

    NSMutableDictionary *query = baseQuery(service, account);
    NSDictionary *attributes = @{(__bridge id)kSecValueData: data};

    // 先尝试更新；不存在则新增
    OSStatus status = SecItemUpdate((__bridge CFDictionaryRef)query,
                                    (__bridge CFDictionaryRef)attributes);
    if (status == errSecItemNotFound) {
        [query addEntriesFromDictionary:attributes];
        status = SecItemAdd((__bridge CFDictionaryRef)query, nullptr);
    }

    if (status != errSecSuccess) {
        if (errorMessage)
            *errorMessage = statusError(status, "写入钥匙串");
        return false;
    }
    return true;
}

QString CredentialStore::read(const QString &service, const QString &account)
{
    NSMutableDictionary *query = baseQuery(service, account);
    query[(__bridge id)kSecReturnData] = @YES;
    query[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;

    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);
    if (status != errSecSuccess || !result)
        return {};

    NSData *data = (__bridge_transfer NSData *)result;
    return QString::fromNSString([[NSString alloc] initWithData:data
                                                       encoding:NSUTF8StringEncoding]);
}

bool CredentialStore::remove(const QString &service, const QString &account)
{
    NSMutableDictionary *query = baseQuery(service, account);
    return SecItemDelete((__bridge CFDictionaryRef)query) == errSecSuccess;
}
