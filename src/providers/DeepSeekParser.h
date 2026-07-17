#pragma once

#include "../core/Models.h"

#include <QByteArray>

// 纯函数：解析 DeepSeek 余额响应（GET /user/balance）。
ProviderSnapshot parseDeepSeekBalance(const QByteArray &body);
