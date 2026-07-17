#include "SettingsDialog.h"

#include "../core/CredentialStore.h"
#include "../core/ProviderManager.h"
#include "../providers/HttpProvider.h"
#include "AddProviderDialog.h"
#include "Theme.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(ProviderManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setWindowTitle(QStringLiteral("设置 · 提供商管理"));
    setMinimumSize(620, 420);

    auto *layout = new QVBoxLayout(this);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *container = new QWidget;
    m_rowsLayout = new QVBoxLayout(container);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(6);
    m_rowsLayout->addStretch();
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea, 1);

    m_statusLabel = new QLabel;
    layout->addWidget(m_statusLabel);

    auto *bottomBar = new QHBoxLayout;
    auto *addButton = new QPushButton(QStringLiteral("＋ 添加提供商"));
    connect(addButton, &QPushButton::clicked, this, &SettingsDialog::addProvider);
    bottomBar->addWidget(addButton);
    bottomBar->addStretch();
    auto *closeButton = new QPushButton(QStringLiteral("关闭"));
    closeButton->setDefault(true);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    bottomBar->addWidget(closeButton);
    layout->addLayout(bottomBar);

    rebuildRows();
    // 管理器数据变化（启停/增删）时同步刷新行。
    // 必须排队连接：启停复选框/删除按钮就在行内，若在它们自己的信号处理链中
    // 同步删除行（=删除发送者本身），信号发射返回到已释放对象 → 闪退。
    // 排队到下一轮事件循环执行，此时控件事件已处理完毕。
    connect(m_manager, &ProviderManager::providersChanged,
            this, &SettingsDialog::rebuildRows, Qt::QueuedConnection);
}

void SettingsDialog::rebuildRows()
{
    // 清空旧行（保留末尾 stretch）
    while (m_rowsLayout->count() > 1) {
        QLayoutItem *item = m_rowsLayout->takeAt(0);
        delete item->widget();
        delete item;
    }

    int row = 0;
    for (const ProviderConfig &config : m_manager->configs())
        m_rowsLayout->insertWidget(row++, buildRow(config));
}

QWidget *SettingsDialog::buildRow(const ProviderConfig &config)
{
    auto *row = new QFrame;
    row->setObjectName(QStringLiteral("providerRow"));
    // 用主题 token 而不是 palette(mid)：暗色下对比度才有保证
    const ThemePalette theme = Theme::current();
    row->setStyleSheet(QStringLiteral(
        "QFrame#providerRow { background: %1; border: 1px solid %2; border-radius: 10px; }"
        "QLabel#muted { color: %3; }")
        .arg(Theme::css(theme.cardBg), Theme::css(theme.border),
             Theme::css(theme.textSecondary)));
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(10, 6, 10, 6);

    // 名称 + 副标题（模板/来源）
    auto *titleBox = new QVBoxLayout;
    titleBox->setSpacing(0);
    titleBox->addWidget(new QLabel(config.name));
    const QString source = config.source == QStringLiteral("user")
        ? QStringLiteral("自定义") : QStringLiteral("内置");
    QString subtitle = source;
    if (config.type == QStringLiteral("demo"))
        subtitle = QStringLiteral("演示数据源");
    else if (!config.parserTemplate.isEmpty())
        subtitle = HttpProvider::templateDisplayName(config.parserTemplate)
                   + QStringLiteral(" · ") + source;
    auto *subtitleLabel = new QLabel(subtitle);
    subtitleLabel->setObjectName(QStringLiteral("muted"));
    titleBox->addWidget(subtitleLabel);
    h->addLayout(titleBox, 1);

    // API Key（http 类型）
    if (config.type == QStringLiteral("http")) {
        const bool hasKey = !CredentialStore::read(credentialServiceFor(config.id),
                                                   config.credentialKey).isEmpty();
        auto *keyEdit = new QLineEdit;
        keyEdit->setEchoMode(QLineEdit::Password);
        keyEdit->setMaximumWidth(180);
        keyEdit->setPlaceholderText(hasKey
            ? QStringLiteral("已保存 ✓（输入新值覆盖）")
            : QStringLiteral("粘贴 API Key"));
        h->addWidget(keyEdit);

        auto *saveButton = new QPushButton(QStringLiteral("保存 Key"));
        saveButton->setEnabled(false);
        connect(keyEdit, &QLineEdit::textChanged, saveButton,
                [saveButton](const QString &text) {
            saveButton->setEnabled(!text.trimmed().isEmpty());
        });
        connect(saveButton, &QPushButton::clicked, this,
                [this, config, keyEdit] { saveKey(config, keyEdit); });
        h->addWidget(saveButton);
    } else {
        auto *noKey = new QLabel(QStringLiteral("无需 Key"));
        noKey->setObjectName(QStringLiteral("muted"));
        h->addWidget(noKey);
    }

    // 启用开关
    auto *enableCheck = new QCheckBox(QStringLiteral("启用"));
    enableCheck->setChecked(config.enabled);
    connect(enableCheck, &QCheckBox::toggled, this,
            [this, config](bool checked) {
        ProviderConfig updated = config;
        updated.enabled = checked;
        m_manager->upsertConfig(updated);
    });
    h->addWidget(enableCheck);

    // 编辑 / 删除（仅用户源）
    if (config.source == QStringLiteral("user")) {
        auto *editButton = new QPushButton(QStringLiteral("编辑"));
        connect(editButton, &QPushButton::clicked, this,
                [this, config] { editProvider(config); });
        h->addWidget(editButton);

        auto *deleteButton = new QPushButton(QStringLiteral("删除"));
        connect(deleteButton, &QPushButton::clicked, this, [this, config] {
            const auto choice = QMessageBox::question(
                this, QStringLiteral("删除提供商"),
                QStringLiteral("确定删除「%1」吗？配置将从用户文件中移除。").arg(config.name));
            if (choice == QMessageBox::Yes) {
                CredentialStore::remove(credentialServiceFor(config.id),
                                        config.credentialKey);
                m_manager->removeConfig(config.id);
            }
        });
        h->addWidget(deleteButton);
    }

    return row;
}

void SettingsDialog::addProvider()
{
    AddProviderDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const ProviderConfig config = dialog.config();
    m_manager->upsertConfig(config);

    if (!dialog.apiKey().isEmpty()) {
        QString error;
        if (CredentialStore::save(credentialServiceFor(config.id), config.credentialKey,
                                  dialog.apiKey(), &error)) {
            m_statusLabel->setText(QStringLiteral("已添加「%1」并保存 Key").arg(config.name));
            emit credentialsChanged();
        } else {
            m_statusLabel->setText(error);
        }
    } else {
        m_statusLabel->setText(QStringLiteral("已添加「%1」，请补充 API Key").arg(config.name));
    }
}

void SettingsDialog::editProvider(const ProviderConfig &config)
{
    AddProviderDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_manager->upsertConfig(dialog.config());

    if (!dialog.apiKey().isEmpty()) {
        QString error;
        if (CredentialStore::save(credentialServiceFor(config.id), config.credentialKey,
                                  dialog.apiKey(), &error)) {
            m_statusLabel->setText(QStringLiteral("已保存「%1」的修改").arg(config.name));
            emit credentialsChanged();
        } else {
            m_statusLabel->setText(error);
        }
    }
}

void SettingsDialog::saveKey(const ProviderConfig &config, QLineEdit *edit)
{
    QString error;
    if (CredentialStore::save(credentialServiceFor(config.id), config.credentialKey,
                              edit->text().trimmed(), &error)) {
        edit->clear();
        edit->setPlaceholderText(QStringLiteral("已保存 ✓（输入新值覆盖）"));
        m_statusLabel->setText(QStringLiteral("「%1」的 Key 已存入 macOS 钥匙串")
                                   .arg(config.name));
        emit credentialsChanged();
    } else {
        m_statusLabel->setText(error);
    }
}
