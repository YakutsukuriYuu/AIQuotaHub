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
            QColor(QStringLiteral("#1b1b1f")),   // windowBg
            QColor(QStringLiteral("#232329")),   // sidebarBg
            QColor(QStringLiteral("#2a2a31")),   // cardBg
            QColor(QStringLiteral("#3d3d46")),   // border
            QColor(QStringLiteral("#f2f2f5")),   // textPrimary
            QColor(QStringLiteral("#9a9aa3")),   // textSecondary
            QColor(QStringLiteral("#0a84ff")),   // accent
            QColor(QStringLiteral("#3a3a42")),   // trackBg
            QColor(QStringLiteral("#0a84ff")),   // hoverBorder
        };
    }
    return {
        QColor(QStringLiteral("#f5f5f7")),   // windowBg
        QColor(QStringLiteral("#ececef")),   // sidebarBg
        QColor(QStringLiteral("#ffffff")),   // cardBg
        QColor(QStringLiteral("#d9d9de")),   // border
        QColor(QStringLiteral("#1d1d1f")),   // textPrimary
        QColor(QStringLiteral("#6e6e73")),   // textSecondary
        QColor(QStringLiteral("#007aff")),   // accent
        QColor(QStringLiteral("#e5e5ea")),   // trackBg
        QColor(QStringLiteral("#007aff")),   // hoverBorder
    };
}

QString Theme::appStyleSheet()
{
    const ThemePalette t = current();
    return QStringLiteral(R"(
QMainWindow, QDialog { background: %1; }
QWidget { color: %2; }
QToolBar { background: %1; border: none; spacing: 8px; padding: 4px; }
QToolBar QToolButton { color: %2; background: transparent; border-radius: 6px; padding: 4px 10px; }
QToolBar QToolButton:hover { background: %3; }
QListWidget { background: %4; border: none; border-radius: 8px; outline: none; padding: 4px; }
QListWidget::item { border-radius: 6px; padding: 6px 8px; color: %2; }
QListWidget::item:selected { background: %5; color: white; }
QListWidget::item:hover:!selected { background: %3; }
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }
QLineEdit { background: %6; border: 1px solid %7; border-radius: 6px; padding: 5px 8px; color: %2; selection-background-color: %5; }
QLineEdit:focus { border: 1px solid %5; }
QPushButton { background: %6; border: 1px solid %7; border-radius: 6px; padding: 5px 14px; color: %2; }
QPushButton:hover { border-color: %5; }
QPushButton:default { background: %5; border-color: %5; color: white; }
QPushButton:disabled { color: %8; }
QComboBox, QSpinBox { background: %6; border: 1px solid %7; border-radius: 6px; padding: 4px 8px; color: %2; }
QGroupBox { border: 1px solid %7; border-radius: 8px; margin-top: 12px; padding-top: 8px; color: %2; }
QGroupBox::title { subcontrol-origin: margin; left: 8px; color: %8; }
QCheckBox { color: %2; }
QMenu { background: %4; border: 1px solid %7; border-radius: 8px; padding: 4px; }
QMenu::item { padding: 5px 20px; border-radius: 4px; color: %2; }
QMenu::item:selected { background: %5; color: white; }
QToolTip { background: %4; color: %2; border: 1px solid %7; padding: 4px 8px; }
)").arg(t.windowBg.name(), t.textPrimary.name(), t.trackBg.name(),
        t.sidebarBg.name(), t.accent.name(), t.cardBg.name(),
        t.border.name(), t.textSecondary.name());
}
