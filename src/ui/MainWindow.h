#pragma once

#include "../core/Models.h"
#include "../core/ProvidersConfig.h"

#include <QHash>
#include <QMainWindow>
#include <QVector>

class Provider;
class ProviderCard;
class RefreshScheduler;
class TrayIcon;
class QListWidget;
class QGridLayout;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createProviders();
    void buildUi();
    void relayoutCards();
    void onSnapshot(const ProviderSnapshot &snapshot);
    void openSettings();
    void updateTraySummary();

    QVector<ProviderConfig> m_configs;
    QVector<Provider *> m_providers;
    QHash<QString, ProviderCard *> m_cards;     // providerId -> 卡片
    QHash<QString, QString> m_names;            // providerId -> 显示名
    QHash<QString, ProviderSnapshot> m_latest;  // providerId -> 最新快照

    RefreshScheduler *m_scheduler = nullptr;
    TrayIcon *m_tray = nullptr;
    QListWidget *m_sidebar = nullptr;
    QGridLayout *m_grid = nullptr;
};
