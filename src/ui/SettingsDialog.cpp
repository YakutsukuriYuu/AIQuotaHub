#include "SettingsDialog.h"

#include "../core/CredentialStore.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const QVector<ProviderConfig> &configs, QWidget *parent)
    : QDialog(parent)
    , m_configs(configs)
{
    setWindowTitle(QStringLiteral("设置"));
    setMinimumWidth(460);

    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    for (const ProviderConfig &config : configs) {
        if (config.type != QStringLiteral("http"))
            continue;   // demo 等类型无需凭据

        auto *edit = new QLineEdit;
        edit->setEchoMode(QLineEdit::Password);
        edit->setPlaceholderText(QStringLiteral("粘贴 API Key 后点“保存”"));
        const bool hasKey = !CredentialStore::read(credentialServiceFor(config.id),
                                                   config.credentialKey).isEmpty();
        if (hasKey)
            edit->setPlaceholderText(QStringLiteral("已保存（输入新值可覆盖）"));
        form->addRow(config.name + QStringLiteral("："), edit);
        m_keyEdits.insert(config.id, edit);
    }
    layout->addLayout(form);

    auto *note = new QLabel(QStringLiteral("API Key 保存在 macOS 钥匙串，不会写入任何文件。\n"
                                           "接口配置生效路径：%1")
                                .arg(ProvidersConfig::resolvedPath()));
    note->setWordWrap(true);
    note->setObjectName(QStringLiteral("muted"));
    layout->addWidget(note);

    m_statusLabel = new QLabel;
    layout->addWidget(m_statusLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save
                                         | QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("关闭"));
    connect(buttons->button(QDialogButtonBox::Save), &QPushButton::clicked,
            this, &SettingsDialog::saveAll);
    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked,
            this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::saveAll()
{
    int saved = 0;
    QString firstError;
    for (auto it = m_keyEdits.constBegin(); it != m_keyEdits.constEnd(); ++it) {
        const QString key = it.value()->text().trimmed();
        if (key.isEmpty())
            continue;

        const ProviderConfig *target = nullptr;
        for (const ProviderConfig &config : m_configs) {
            if (config.id == it.key()) {
                target = &config;
                break;
            }
        }
        if (!target)
            continue;

        QString error;
        if (CredentialStore::save(credentialServiceFor(it.key()), target->credentialKey,
                                  key, &error)) {
            ++saved;
            it.value()->clear();
            it.value()->setPlaceholderText(QStringLiteral("已保存（输入新值可覆盖）"));
        } else if (firstError.isEmpty()) {
            firstError = error;
        }
    }

    if (!firstError.isEmpty())
        m_statusLabel->setText(firstError);
    else if (saved > 0)
        m_statusLabel->setText(QStringLiteral("已保存 %1 个 Key 到 macOS 钥匙串").arg(saved));
    else
        m_statusLabel->setText(QStringLiteral("没有需要保存的内容"));
}
