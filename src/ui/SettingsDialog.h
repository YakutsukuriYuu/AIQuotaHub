#pragma once

#include "../core/ProvidersConfig.h"

#include <QDialog>

class ProviderManager;
class QVBoxLayout;
class QLabel;
class QLineEdit;

// 提供商管理页：增删改、启停、API Key 管理（即时生效）
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(ProviderManager *manager, QWidget *parent = nullptr);

signals:
    void credentialsChanged();   // 有 Key 更新（主窗口触发立即重刷）

private:
    void rebuildRows();
    QWidget *buildRow(const ProviderConfig &config);
    void addProvider();
    void editProvider(const ProviderConfig &config);
    void saveKey(const ProviderConfig &config, QLineEdit *edit);

    ProviderManager *m_manager;
    QVBoxLayout *m_rowsLayout = nullptr;
    QLabel *m_statusLabel = nullptr;
};
