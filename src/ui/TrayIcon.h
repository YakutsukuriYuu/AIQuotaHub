#pragma once

#include <QSystemTrayIcon>

class QMenu;
class QAction;

// 菜单栏托盘：图标颜色 = 当前最紧张的额度状态；菜单提供摘要/刷新/退出
class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

    void setStatus(double worstPercent, const QStringList &summaryLines);

signals:
    void showWindowRequested();
    void refreshRequested();

private:
    static QIcon makeIcon(const QColor &color);

    QMenu *m_menu = nullptr;
    QAction *m_summaryAction = nullptr;
};
