#pragma once

#include "../core/Models.h"

#include <QByteArray>

// 纯函数：解析 OpenAI 组织费用响应（GET /v1/organization/costs，需 Admin Key）。
// 聚合所有 bucket 的 results[].amount.value 为当月总花费。
ProviderSnapshot parseOpenAiCosts(const QByteArray &body, const QString &providerId);
