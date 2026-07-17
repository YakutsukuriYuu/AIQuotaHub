#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>
#include <optional>

// 一段配额窗口（5 小时窗 / 周窗）
struct QuotaWindow {
    enum class Kind { FiveHour, Weekly };

    Kind kind = Kind::FiveHour;
    QString label;        // 展示名，如 "5小时额度"
    double used = 0;
    double limit = 0;     // 0 = 未知
    QDateTime resetAt;

    double percent() const { return limit > 0 ? used / limit : 0.0; }
    bool isValid() const { return limit > 0; }
};

// 按量计费 API 的用量/余额
struct ApiUsage {
    QString currency;     // "CNY" / "USD"
    QString label;        // "余额" / "本月花费"
    double balance = 0;
    QString note;         // 附加说明（如含赠送金额、余额不足）
    bool isValid = false;
};

// 一次抓取的完整快照
struct ProviderSnapshot {
    QString providerId;
    QDateTime fetchedAt;
    QVector<QuotaWindow> quotas;
    std::optional<ApiUsage> api;
    QString error;        // 非空 = 抓取失败

    bool ok() const { return error.isEmpty(); }

    // 最紧张的配额百分比，用于卡片/托盘着色；无有效配额返回 -1
    double worstQuotaPercent() const
    {
        double worst = -1.0;
        for (const auto &quota : quotas) {
            if (quota.isValid())
                worst = qMax(worst, quota.percent());
        }
        return worst;
    }
};
