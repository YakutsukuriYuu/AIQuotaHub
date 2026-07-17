#include "ProviderCard.h"

#include "UiColors.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

namespace {

void clearLayout(QLayout *layout)
{
    while (QLayoutItem *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

} // namespace

ProviderCard::ProviderCard(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("ProviderCard"));
    setStyleSheet(QStringLiteral(
        "QFrame#ProviderCard { background: palette(base); border: 1px solid palette(mid);"
        " border-radius: 10px; }"
        "QLabel#cardTitle { font-weight: bold; font-size: 15px; }"
        "QLabel#muted { color: palette(mid); }"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 10, 14, 12);

    auto *header = new QHBoxLayout;
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("muted"));
    header->addWidget(titleLabel);
    header->addStretch();
    header->addWidget(m_statusLabel);
    root->addLayout(header);

    m_bodyLayout = new QVBoxLayout;
    m_bodyLayout->setSpacing(8);
    root->addLayout(m_bodyLayout);

    setMinimumWidth(330);
    showLoading();
}

void ProviderCard::showLoading()
{
    clearLayout(m_bodyLayout);
    auto *label = new QLabel(QStringLiteral("加载中…"));
    label->setObjectName(QStringLiteral("muted"));
    m_bodyLayout->addWidget(label);
}

void ProviderCard::updateSnapshot(const ProviderSnapshot &snapshot)
{
    clearLayout(m_bodyLayout);
    m_statusLabel->setText(snapshot.fetchedAt.toString(QStringLiteral("HH:mm:ss")));

    if (!snapshot.ok()) {
        auto *label = new QLabel(QStringLiteral("⚠ ") + snapshot.error);
        label->setWordWrap(true);
        m_bodyLayout->addWidget(label);
        return;
    }

    for (const QuotaWindow &quota : snapshot.quotas)
        m_bodyLayout->addWidget(buildQuotaRow(quota));

    if (snapshot.api && snapshot.api->isValid) {
        const ApiUsage &api = *snapshot.api;
        QString text = QStringLiteral("%1：%2 %3")
                           .arg(api.label, QString::number(api.balance, 'f', 2), api.currency);
        if (!api.note.isEmpty())
            text += QStringLiteral("（%1）").arg(api.note);
        m_bodyLayout->addWidget(new QLabel(text));
    }

    if (snapshot.quotas.isEmpty() && !snapshot.api)
        m_bodyLayout->addWidget(new QLabel(QStringLiteral("无数据")));
}

QWidget *ProviderCard::buildQuotaRow(const QuotaWindow &quota)
{
    auto *row = new QWidget;
    auto *v = new QVBoxLayout(row);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(3);

    const int percent = qRound(quota.percent() * 100);

    auto *line = new QHBoxLayout;
    line->addWidget(new QLabel(quota.label));
    line->addStretch();
    line->addWidget(new QLabel(QStringLiteral("%1%").arg(percent)));
    if (quota.resetAt.isValid()) {
        auto *reset = new QLabel(quota.resetAt.toString(QStringLiteral("HH:mm"))
                                 + QStringLiteral(" 重置"));
        reset->setObjectName(QStringLiteral("muted"));
        line->addWidget(reset);
    }
    v->addLayout(line);

    auto *bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(percent);
    bar->setTextVisible(false);
    bar->setFixedHeight(6);
    bar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: none; border-radius: 3px; background: palette(midlight); }"
        "QProgressBar::chunk { border-radius: 3px; background: %1; }")
        .arg(statusColorForPercent(quota.percent()).name()));
    v->addWidget(bar);

    return row;
}
