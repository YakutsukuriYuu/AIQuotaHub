#pragma once

#include "../core/Models.h"

#include <QFrame>

class QLabel;
class QVBoxLayout;

// 单个提供商的仪表盘卡片：配额进度条 + API 余额 + 状态着色
class ProviderCard : public QFrame
{
    Q_OBJECT
public:
    explicit ProviderCard(const QString &title, QWidget *parent = nullptr);

    void showLoading();
    void updateSnapshot(const ProviderSnapshot &snapshot);

private:
    QWidget *buildQuotaRow(const QuotaWindow &quota);

    QLabel *m_statusLabel = nullptr;      // 右上角：最近更新时间
    QVBoxLayout *m_bodyLayout = nullptr;  // 动态内容区
};
