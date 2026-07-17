#pragma once

#include "../core/ProvidersConfig.h"

#include <QDialog>
#include <QHash>

class QLineEdit;
class QLabel;

// 设置对话框：录入各提供商 API Key，写入 macOS 钥匙串
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const QVector<ProviderConfig> &configs, QWidget *parent = nullptr);

private:
    void saveAll();

    QVector<ProviderConfig> m_configs;
    QHash<QString, QLineEdit *> m_keyEdits;   // providerId -> 输入框
    QLabel *m_statusLabel = nullptr;
};
