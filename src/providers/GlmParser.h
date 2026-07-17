#pragma once

#include "../core/Models.h"

#include <QByteArray>
#include <QJsonObject>

// 纯函数：解析智谱配额响应（GET /api/monitor/usage/quota/limit）。
// 独立成纯函数便于单测，不碰网络。
// fields 可覆盖字段路径（来自 providers.json 的 "fields" 节），目前支持：
//   "limitsArray": 配额数组的点分路径，默认 "data.limits"
ProviderSnapshot parseGlmQuotaResponse(const QByteArray &body, const QJsonObject &fields,
                                       const QString &providerId);
