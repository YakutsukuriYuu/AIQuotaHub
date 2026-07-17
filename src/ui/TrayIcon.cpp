#include "TrayIcon.h"

#include "UiColors.h"

#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QPainter>

TrayIcon::TrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
    , m_menu(new QMenu)
{
    m_summaryAction = m_menu->addAction(QStringLiteral("暂无数据"));
    m_summaryAction->setEnabled(false);
    m_menu->addSeparator();
    m_menu->addAction(QStringLiteral("立即刷新"), this, &TrayIcon::refreshRequested);
    m_menu->addAction(QStringLiteral("显示主窗口"), this, &TrayIcon::showWindowRequested);
    m_menu->addSeparator();
    m_menu->addAction(QStringLiteral("退出"), qApp, &QCoreApplication::quit);

    setIcon(makeIcon(statusColorForPercent(-1)));
    setContextMenu(m_menu);
    setToolTip(QStringLiteral("AI Quota Hub"));

    connect(this, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            emit showWindowRequested();
    });
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::setStatus(double worstPercent, const QStringList &summaryLines)
{
    setIcon(makeIcon(statusColorForPercent(worstPercent)));
    m_summaryAction->setText(summaryLines.isEmpty()
                                 ? QStringLiteral("暂无数据")
                                 : summaryLines.join(QStringLiteral("  ·  ")));
    setToolTip(summaryLines.isEmpty() ? QStringLiteral("AI Quota Hub")
                                      : summaryLines.join(u'\n'));
}

QIcon TrayIcon::makeIcon(const QColor &color)
{
    QPixmap pixmap(44, 44);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color.darker(125), 3));
    painter.setBrush(color);
    painter.drawEllipse(6, 6, 32, 32);

    return QIcon(pixmap);
}
