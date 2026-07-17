#pragma once

#include "../core/Models.h"

#include <QByteArray>
#include <QJsonObject>

// 纯函数：按用户自定义的字段路径映射解析任意 JSON 响应。
// fields 支持的键：
//   "quotasArray"  配额数组点分路径（可空）
//   "used"/"limit"/"reset"  配额条目内的键名（缺省用内置候选）
//   "quotaLabel"   配额展示名（可空）
//   "weeklyType"   type 字段等于该值时判为周窗（可空，默认含 "WEEK" 即周窗）
//   "balance"      余额点分路径（可空；数字或数字字符串）
//   "currency"     币种点分路径（可空）
//   "currencyFixed" 固定币种（可空，默认 "CNY"）
//   "balanceLabel" 余额展示名（可空，默认 "余额"）
ProviderSnapshot parseCustomJsonResponse(const QByteArray &body, const QJsonObject &fields,
                                         const QString &providerId);
