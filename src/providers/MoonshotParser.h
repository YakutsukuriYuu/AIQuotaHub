#pragma once

#include "../core/Models.h"

#include <QByteArray>

// 纯函数：解析月之暗面（Kimi）余额响应（GET /v1/users/me/balance）。
// 响应形态：{"code":0, "data":{"available_balance":…, "voucher_balance":…, "cash_balance":…}, "status":true}
ProviderSnapshot parseMoonshotBalance(const QByteArray &body, const QString &providerId);
