#pragma once

#include "../core/Models.h"

#include <QHash>
#include <QMainWindow>

class ProviderManager;
class ProviderCard;
class RefreshScheduler;
class TrayIcon;
class QListWidget;
class QGridLayout;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void rebuildDashboard();   // providersChanged 后重建卡片/侧栏/调度
    void relayoutCards();
    void onSnapshot(const ProviderSnapshot &snapshot);
    void openSettings();
    void updateTraySummary();
    void updateHeader();         // 头部栏副标题（数据源数 + 最近更新）
    void updateSidebarStatus();  // 侧栏每行状态点
    void applyThemeToUi();       // 系统明暗切换后重刷全局样式和卡片

    ProviderManager *m_manager = nullptr;
    RefreshScheduler *m_scheduler = nullptr;
    TrayIcon *m_tray = nullptr;
    QListWidget *m_sidebar = nullptr;
    QGridLayout *m_grid = nullptr;
    QLabel *m_headerSubtitle = nullptr;
    QPushButton *m_refreshButton = nullptr;

    QHash<QString, ProviderCard *> m_cards;     // providerId -> 卡片
    QHash<QString, ProviderSnapshot> m_latest;  // providerId -> 最新快照
};
