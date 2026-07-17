#include "RingProgress.h"

#include "Theme.h"
#include "UiColors.h"

#include <QPainter>

RingProgress::RingProgress(QWidget *parent)
    : QWidget(parent)
{
}

void RingProgress::setValue(double percent)
{
    m_percent = percent;
    update();
}

void RingProgress::setSubText(const QString &text)
{
    m_subText = text;
    update();
}

void RingProgress::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const ThemePalette theme = Theme::current();
    const int penWidth = 7;
    const int side = qMin(width(), height());
    const QRectF rect((width() - side) / 2.0 + penWidth,
                      (height() - side) / 2.0 + penWidth,
                      side - 2.0 * penWidth, side - 2.0 * penWidth);

    // 轨道环
    painter.setPen(QPen(theme.trackBg, penWidth, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect);

    // 进度弧（12 点方向起，顺时针）
    if (m_percent >= 0) {
        painter.setPen(QPen(statusColorForPercent(m_percent), penWidth,
                            Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(rect, 90 * 16, int(-360 * 16 * qMin(m_percent, 1.0)));
    }

    // 中心百分比
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(15);
    painter.setFont(font);
    painter.setPen(theme.textPrimary);
    const QString centerText = m_percent < 0
        ? QStringLiteral("–")
        : QStringLiteral("%1%").arg(qRound(m_percent * 100));
    painter.drawText(m_subText.isEmpty() ? rect : rect.adjusted(0, -6, 0, 0),
                     Qt::AlignCenter, centerText);

    // 环内下方小字
    if (!m_subText.isEmpty()) {
        font.setBold(false);
        font.setPixelSize(8);
        painter.setFont(font);
        painter.setPen(theme.textSecondary);
        painter.drawText(QRectF(rect.x(), rect.center().y() + 2,
                                rect.width(), rect.height() / 2 - 2),
                         Qt::AlignHCenter | Qt::AlignTop, m_subText);
    }
}
