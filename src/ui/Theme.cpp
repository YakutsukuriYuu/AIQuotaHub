#include "Theme.h"

#include <QGuiApplication>
#include <QStyleHints>

bool Theme::isDark()
{
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

ThemePalette Theme::current()
{
    if (isDark()) {
        return {
            QColor(QStringLiteral("#161619")),       // windowBg
            QColor(QStringLiteral("#1e1e22")),       // sidebarBg
            QColor(QStringLiteral("#232328")),       // cardBg
            QColor(255, 255, 255, 24),               // border（半透明）
            QColor(QStringLiteral("#f5f5f7")),       // textPrimary
            QColor(QStringLiteral("#98989f")),       // textSecondary
            QColor(QStringLiteral("#0a84ff")),       // accent
            QColor(255, 255, 255, 18),               // trackBg（半透明）
            QColor(10, 132, 255, 160),               // hoverBorder
            QColor(0, 0, 0, 110),                    // shadow
        };
    }
    return {
        QColor(QStringLiteral("#f2f2f6")),       // windowBg
        QColor(QStringLiteral("#eaeaef")),       // sidebarBg
        QColor(QStringLiteral("#ffffff")),       // cardBg
        QColor(0, 0, 0, 18),                     // border（半透明）
        QColor(QStringLiteral("#1d1d1f")),       // textPrimary
        QColor(QStringLiteral("#6e6e73")),       // textSecondary
        QColor(QStringLiteral("#007aff")),       // accent
        QColor(0, 0, 0, 12),                     // trackBg（半透明）
        QColor(0, 122, 255, 150),                // hoverBorder
        QColor(0, 0, 0, 40),                     // shadow
    };
}

QString Theme::appStyleSheet()
{
    const ThemePalette t = current();
    return QStringLiteral(R"(
QMainWindow, QDialog { background: %1; }
QWidget { color: %2; }

QLabel#appHeaderTitle { font-size: 20px; font-weight: bold; color: %2; }
QLabel#appHeaderSubtitle { color: %8; font-size: 12px; }
QLabel#sidebarSection { color: %8; font-size: 11px; font-weight: bold; padding: 2px 8px; }

QPushButton#primaryButton { background: %5; color: white; border: none; border-radius: 8px; padding: 6px 18px; font-weight: bold; }
QPushButton#primaryButton:hover { background: %9; }
QPushButton#primaryButton:pressed { background: %5; }
QPushButton#secondaryButton { background: %6; border: 1px solid %7; border-radius: 8px; padding: 6px 16px; color: %2; }
QPushButton#secondaryButton:hover { border-color: %5; }

QListWidget { background: %4; border: none; border-radius: 10px; outline: none; padding: 4px; }
QListWidget::item { border-radius: 6px; padding: 7px 10px; color: %2; }
QListWidget::item:selected { background: %5; color: white; }
QListWidget::item:hover:!selected { background: %3; }

QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }

QLineEdit { background: %6; border: 1px solid %7; border-radius: 8px; padding: 6px 10px; color: %2; selection-background-color: %5; }
QLineEdit:focus { border: 1px solid %5; }
QPushButton { background: %6; border: 1px solid %7; border-radius: 8px; padding: 6px 14px; color: %2; }
QPushButton:hover { border-color: %5; }
QPushButton:default { background: %5; border-color: %5; color: white; }
QPushButton:disabled { color: %8; }
QComboBox, QSpinBox { background: %6; border: 1px solid %7; border-radius: 8px; padding: 5px 8px; color: %2; }
QGroupBox { border: 1px solid %7; border-radius: 10px; margin-top: 12px; padding-top: 8px; color: %2; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; color: %8; }
QCheckBox { color: %2; }
QMenu { background: %4; border: 1px solid %7; border-radius: 10px; padding: 4px; }
QMenu::item { padding: 5px 20px; border-radius: 6px; color: %2; }
QMenu::item:selected { background: %5; color: white; }
QToolTip { background: %4; color: %2; border: 1px solid %7; padding: 4px 8px; }
)")
        .arg(css(t.windowBg), css(t.textPrimary), css(t.trackBg), css(t.sidebarBg),
             css(t.accent), css(t.cardBg), css(t.border), css(t.textSecondary),
             css(t.accent.lighter(115)));
}
