#pragma once

#include <QColor>
#include <QString>

// 主题调色板 token：所有界面颜色从这里取，禁止散落硬编码。
// border/trackBg 带 alpha，叠在背景上更柔和（输出时用 Theme::css 保留透明度）。
struct ThemePalette {
    QColor windowBg;       // 窗口背景
    QColor sidebarBg;      // 侧栏/菜单背景
    QColor cardBg;         // 卡片背景
    QColor border;         // 常规边框（半透明）
    QColor textPrimary;
    QColor textSecondary;
    QColor accent;         // 强调色（选中、主按钮、focus）
    QColor trackBg;        // 圆环轨道、hover 底色（半透明）
    QColor hoverBorder;    // 卡片 hover 边框
    QColor shadow;         // 卡片投影
};

// 跟随 macOS 系统外观的明暗双主题
class Theme
{
public:
    static bool isDark();               // QStyleHints::colorScheme 检测
    static ThemePalette current();      // 当前调色板
    static QString appStyleSheet();     // 全套 QSS（基于 current()）

    // 颜色 → QSS 字符串（保留 alpha）
    static QString css(const QColor &color)
    {
        return color.alpha() < 255 ? color.name(QColor::HexArgb) : color.name();
    }
};
