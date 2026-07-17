#pragma once

#include <QColor>

// 用量百分比 → 状态色（macOS 系统色风格）：<60% 绿 / 60–85% 黄 / >85% 红 / 负值灰
inline QColor statusColorForPercent(double percent)
{
    if (percent < 0)
        return QColor(0x8e, 0x8e, 0x93);
    if (percent < 0.60)
        return QColor(0x34, 0xc7, 0x59);
    if (percent < 0.85)
        return QColor(0xff, 0x9f, 0x0a);
    return QColor(0xff, 0x3b, 0x30);
}
