#include "ProviderCard.h"

#include "RingProgress.h"
#include "Theme.h"
#include "UiColors.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

void clearLayout(QLayout *layout)
{
    while (QLayoutItem *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

QString cardStyleSheet()
{
    const ThemePalette t = Theme::current();
    return QStringLiteral(
        "QFrame#ProviderCard { background: %1; border: 1px solid %2; border-radius: 12px; }"
        "QFrame#ProviderCard:hover { border: 1px solid %3; }"
        "QLabel#cardTitle { font-size: 15px; font-weight: 600; color: %4; }"
        "QLabel#muted { color: %5; font-size: 11px; }"
        "QLabel#quotaLabel { color: %5; font-size: 11px; }"
        "QLabel#balanceText { font-size: 13px; color: %4; }")
        .arg(t.cardBg.name(), t.border.name(), t.hoverBorder.name(),
             t.textPrimary.name(), t.textSecondary.name());
}

} // namespace

ProviderCard::ProviderCard(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ProviderCard"));
    setAttribute(Qt::WA_Hover, true);   // 启用 QSS :hover 伪态

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 12, 0);
    root->setSpacing(10);

    m_statusBar = new QFrame;
    m_statusBar->setFixedWidth(4);
    root->addWidget(m_statusBar);

    auto *content = new QVBoxLayout;
    content->setContentsMargins(0, 10, 0, 12);
    content->setSpacing(8);
    root->addLayout(content, 1);

    auto *header = new QHBoxLayout;
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("muted"));
    header->addWidget(titleLabel);
    header->addStretch();
    header->addWidget(m_statusLabel);
    content->addLayout(header);

    m_bodyLayout = new QVBoxLayout;
    m_bodyLayout->setSpacing(6);
    content->addLayout(m_bodyLayout);

    applyTheme();
    setMinimumWidth(340);
    showLoading();
}

void ProviderCard::applyTheme()
{
    setStyleSheet(cardStyleSheet());
    if (m_snapshot.has_value())
        updateSnapshot(*m_snapshot);   // 用最新数据重绘（圆环颜色取自新主题）
}

void ProviderCard::setStatusColor(const QColor &color)
{
    m_statusBar->setStyleSheet(QStringLiteral(
        "background: %1; border-radius: 2px;").arg(color.name()));
}

void ProviderCard::showLoading()
{
    clearLayout(m_bodyLayout);
    auto *label = new QLabel(QStringLiteral("加载中…"));
    label->setObjectName(QStringLiteral("muted"));
    m_bodyLayout->addWidget(label);
    setStatusColor(statusColorForPercent(-1));
}

void ProviderCard::updateSnapshot(const ProviderSnapshot &snapshot)
{
    m_snapshot = snapshot;
    clearLayout(m_bodyLayout);
    m_statusLabel->setText(snapshot.fetchedAt.toString(QStringLiteral("HH:mm:ss")));

    if (!snapshot.ok()) {
        setStatusColor(statusColorForPercent(-1));
        auto *label = new QLabel(QStringLiteral("⚠ ") + snapshot.error);
        label->setObjectName(QStringLiteral("muted"));
        label->setWordWrap(true);
        m_bodyLayout->addWidget(label);
        return;
    }

    setStatusColor(statusColorForPercent(snapshot.worstQuotaPercent()));

    if (!snapshot.quotas.isEmpty()) {
        auto *ringsRow = new QWidget;
        auto *h = new QHBoxLayout(ringsRow);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(20);
        for (const QuotaWindow &quota : snapshot.quotas)
            h->addWidget(buildQuotaRing(quota));
        h->addStretch();
        m_bodyLayout->addWidget(ringsRow);
    }

    if (snapshot.api && snapshot.api->isValid) {
        const ApiUsage &api = *snapshot.api;
        QString text = QStringLiteral("%1：%2 %3")
                           .arg(api.label, QString::number(api.balance, 'f', 2), api.currency);
        if (!api.note.isEmpty())
            text += QStringLiteral("（%1）").arg(api.note);
        auto *label = new QLabel(text);
        label->setObjectName(QStringLiteral("balanceText"));
        m_bodyLayout->addWidget(label);
    }

    if (snapshot.quotas.isEmpty() && !snapshot.api) {
        auto *label = new QLabel(QStringLiteral("无数据"));
        label->setObjectName(QStringLiteral("muted"));
        m_bodyLayout->addWidget(label);
    }
}

QWidget *ProviderCard::buildQuotaRing(const QuotaWindow &quota)
{
    auto *box = new QWidget;
    auto *v = new QVBoxLayout(box);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    auto *ring = new RingProgress;
    ring->setValue(quota.percent());
    ring->setSubText(QStringLiteral("%1/%2")
                         .arg(qRound64(quota.used))
                         .arg(qRound64(quota.limit)));
    v->addWidget(ring, 0, Qt::AlignHCenter);

    auto *label = new QLabel(quota.label);
    label->setObjectName(QStringLiteral("quotaLabel"));
    label->setAlignment(Qt::AlignCenter);
    v->addWidget(label);

    if (quota.resetAt.isValid()) {
        auto *reset = new QLabel(quota.resetAt.toString(QStringLiteral("HH:mm"))
                                 + QStringLiteral(" 重置"));
        reset->setObjectName(QStringLiteral("quotaLabel"));
        reset->setAlignment(Qt::AlignCenter);
        v->addWidget(reset);
    }

    return box;
}
