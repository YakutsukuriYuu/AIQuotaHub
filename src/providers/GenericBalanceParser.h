#pragma once

#include "../core/Models.h"

#include <QByteArray>
#include <QJsonObject>

// 纯函数：通用余额解析。按优先级自动识别常见响应结构：
//   A. DeepSeek 形态：{"balance_infos": [{"total_balance": "…", ...}]}
//   B. Moonshot 形态：{"code": 0, "data": {"available_balance": …}}
//   C. 自定义路径：fields["balance"] / fields["currency"] 指定的点分路径
// 都识别不了时给出明确错误。
ProviderSnapshot parseGenericBalance(const QByteArray &body, const QJsonObject &fields,
                                     const QString &providerId);
