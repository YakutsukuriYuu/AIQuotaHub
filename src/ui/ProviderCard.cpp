#include "ProviderCard.h"

#include "RingProgress.h"
#include "Theme.h"
#include "UiColors.h"

#include <QGraphicsDropShadowEffect>
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
        "QFrame#ProviderCard { background: %1; border: 1px solid %2; border-radius: 14px; }"
        "QFrame#ProviderCard:hover { border: 1px solid %3; }"
        "QLabel#cardTitle { font-size: 15px; font-weight: 600; color: %4; }"
        "QLabel#muted { color: %5; font-size: 11px; }"
        "QLabel#quotaLabel { color: %5; font-size: 11px; }"
        "QLabel#balanceValue { font-size: 14px; font-weight: 600; color: %4; }")
        .arg(Theme::css(t.cardBg), Theme::css(t.border), Theme::css(t.hoverBorder),
             Theme::css(t.textPrimary), Theme::css(t.textSecondary));
}

} // namespace

ProviderCard::ProviderCard(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ProviderCard"));
    setAttribute(Qt::WA_Hover, true);   // 启用 QSS :hover 伪态

    // 柔和投影（颜色随主题，在 applyTheme 里更新）
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 14, 0);
    root->setSpacing(12);

    m_statusBar = new QFrame;
    m_statusBar->setFixedWidth(4);
    root->addWidget(m_statusBar);

    auto *content = new QVBoxLayout;
    content->setContentsMargins(0, 12, 0, 12);
    content->setSpacing(8);
    root->addLayout(content, 1);

    auto *header = new QHBoxLayout;
    header->setSpacing(8);
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    m_pill = new QLabel;
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("muted"));
    header->addWidget(titleLabel);
    header->addStretch();
    header->addWidget(m_pill);
    header->addWidget(m_statusLabel);
    content->addLayout(header);

    m_bodyLayout = new QVBoxLayout;
    m_bodyLayout->setSpacing(8);
    content->addLayout(m_bodyLayout);

    applyTheme();
    setMinimumWidth(340);
    showLoading();
}

void ProviderCard::applyTheme()
{
    setStyleSheet(cardStyleSheet());
    if (auto *effect = qobject_cast<QGraphicsDropShadowEffect *>(graphicsEffect()))
        effect->setColor(Theme::current().shadow);
    if (m_snapshot.has_value())
        updateSnapshot(*m_snapshot);   // 用最新数据重绘（圆环颜色取自新主题）
}

void ProviderCard::setStatusColor(const QColor &color)
{
    m_statusBar->setStyleSheet(QStringLiteral(
        "background: %1; border-radius: 2px;").arg(color.name()));
}

void ProviderCard::setPill(const QString &text, const QColor &color)
{
    QColor background = color;
    background.setAlpha(40);
    m_pill->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; color: %2; border-radius: 9px;"
        " padding: 2px 9px; font-size: 11px; font-weight: bold; }")
        .arg(background.name(QColor::HexArgb), color.name()));
    m_pill->setText(text);
}

void ProviderCard::showLoading()
{
    clearLayout(m_bodyLayout);
    auto *label = new QLabel(QStringLiteral("加载中…"));
    label->setObjectName(QStringLiteral("muted"));
    m_bodyLayout->addWidget(label);
    setStatusColor(statusColorForPercent(-1));
    setPill(QStringLiteral("加载中"), statusColorForPercent(-1));
}

void ProviderCard::updateSnapshot(const ProviderSnapshot &snapshot)
{
    m_snapshot = snapshot;
    clearLayout(m_bodyLayout);
    m_statusLabel->setText(snapshot.fetchedAt.toString(QStringLiteral("HH:mm:ss")));

    if (!snapshot.ok()) {
        setStatusColor(statusColorForPercent(1.0));
        setPill(QStringLiteral("失败"), statusColorForPercent(1.0));
        auto *label = new QLabel(QStringLiteral("⚠ ") + snapshot.error);
        label->setObjectName(QStringLiteral("muted"));
        label->setWordWrap(true);
        m_bodyLayout->addWidget(label);
        return;
    }

    const double worst = snapshot.worstQuotaPercent();
    const QColor status = statusColorForPercent(worst);
    setStatusColor(status);
    if (worst < 0)
        setPill(QStringLiteral("正常"), statusColorForPercent(0.0));
    else if (worst < 0.60)
        setPill(QStringLiteral("正常"), status);
    else if (worst < 0.85)
        setPill(QStringLiteral("偏高"), status);
    else
        setPill(QStringLiteral("紧张"), status);

    if (!snapshot.quotas.isEmpty()) {
        auto *ringsRow = new QWidget;
        auto *h = new QHBoxLayout(ringsRow);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(24);
        h->addStretch();
        for (const QuotaWindow &quota : snapshot.quotas)
            h->addWidget(buildQuotaRing(quota));
        h->addStretch();
        m_bodyLayout->addWidget(ringsRow);
    }

    if (snapshot.api && snapshot.api->isValid) {
        const ApiUsage &api = *snapshot.api;

        // 分隔线
        auto *divider = new QFrame;
        divider->setFixedHeight(1);
        divider->setStyleSheet(QStringLiteral("background: %1;")
                                   .arg(Theme::css(Theme::current().border)));
        m_bodyLayout->addWidget(divider);

        // 余额行：左标签，右数值（半粗），备注小字
        auto *row = new QWidget;
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        auto *label = new QLabel(api.label);
        label->setObjectName(QStringLiteral("muted"));
        h->addWidget(label);
        h->addStretch();
        auto *value = new QLabel(QStringLiteral("%2 %1")
                                     .arg(api.currency)
                                     .arg(api.balance, 0, 'f', 2));
        value->setObjectName(QStringLiteral("balanceValue"));
        h->addWidget(value);
        if (!api.note.isEmpty()) {
            auto *note = new QLabel(QStringLiteral("（%1）").arg(api.note));
            note->setObjectName(QStringLiteral("muted"));
            h->addWidget(note);
        }
        m_bodyLayout->addWidget(row);
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
    v->setSpacing(3);

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
