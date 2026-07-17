#pragma once

#include "../core/Models.h"

#include <QFrame>
#include <optional>

class QLabel;
class QVBoxLayout;

// 单个提供商的仪表盘卡片：投影 + 状态徽章 + 圆环进度 + API 余额 + hover 提亮
class ProviderCard : public QFrame
{
    Q_OBJECT
public:
    explicit ProviderCard(const QString &title, QWidget *parent = nullptr);

    void showLoading();
    void updateSnapshot(const ProviderSnapshot &snapshot);
    void applyTheme();   // 主题切换后重刷样式并重绘

private:
    QWidget *buildQuotaRing(const QuotaWindow &quota);
    void setStatusColor(const QColor &color);
    void setPill(const QString &text, const QColor &color);   // 右上角状态徽章

    std::optional<ProviderSnapshot> m_snapshot;   // 主题切换时重绘用
    QFrame *m_statusBar = nullptr;                // 左侧状态色条
    QLabel *m_pill = nullptr;                     // 状态徽章
    QLabel *m_statusLabel = nullptr;              // 最近更新时间
    QVBoxLayout *m_bodyLayout = nullptr;
};
