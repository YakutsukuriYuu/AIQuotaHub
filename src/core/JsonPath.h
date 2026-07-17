#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <initializer_list>

// 点分路径取值："data.limits"
inline QJsonValue jsonValueAtPath(const QJsonValue &root, const QString &path)
{
    QJsonValue current = root;
    const QStringList segments = path.split(u'.', Qt::SkipEmptyParts);
    for (const QString &segment : segments) {
        if (!current.isObject())
            return {};
        current = current.toObject().value(segment);
    }
    return current;
}

// 依次尝试多个候选键，返回第一个存在的（容忍接口字段名漂移）
inline QJsonValue jsonFirstOf(const QJsonObject &object, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const auto it = object.constFind(QLatin1StringView(key));
        if (it != object.constEnd())
            return it.value();
    }
    return {};
}
