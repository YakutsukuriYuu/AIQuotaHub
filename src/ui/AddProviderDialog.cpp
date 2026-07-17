#include "AddProviderDialog.h"

#include "../providers/HttpProvider.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

// 各模板默认接口地址（选模板时预填；余额型格式通用，只给示例不预填）
QString defaultEndpointFor(const QString &parserTemplate)
{
    if (parserTemplate == QStringLiteral("quota_windows"))
        return QStringLiteral("https://open.bigmodel.cn/api/monitor/usage/quota/limit");
    if (parserTemplate == QStringLiteral("openai_costs"))
        return QStringLiteral(
            "https://api.openai.com/v1/organization/costs?start_time={month_start}");
    return {};
}

// 各模板 URL 输入框的占位提示
QString endpointPlaceholderFor(const QString &parserTemplate)
{
    if (parserTemplate == QStringLiteral("balance_json"))
        return QStringLiteral("https://…（例如 https://api.deepseek.com/user/balance）");
    if (parserTemplate == QStringLiteral("quota_windows"))
        return QStringLiteral("配额查询接口地址");
    if (parserTemplate == QStringLiteral("openai_costs"))
        return QStringLiteral("OpenAI 兼容的用量/费用接口地址");
    return QStringLiteral("https://…");
}

// custom_json 高级区暴露的字段映射键
const char *kCustomFieldKeys[] = {
    "quotasArray", "used", "limit", "reset", "balance", "currency",
};

} // namespace

AddProviderDialog::AddProviderDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("添加提供商"));
    buildUi();
}

AddProviderDialog::AddProviderDialog(const ProviderConfig &existing, QWidget *parent)
    : QDialog(parent)
    , m_original(existing)
    , m_editMode(true)
{
    setWindowTitle(QStringLiteral("编辑提供商"));
    buildUi();
}

void AddProviderDialog::buildUi()
{
    setMinimumWidth(480);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_nameEdit = new QLineEdit(m_original.name);
    m_nameEdit->setPlaceholderText(QStringLiteral("例如：我的 GLM"));
    form->addRow(QStringLiteral("名称："), m_nameEdit);

    m_templateCombo = new QComboBox;
    const auto templates = HttpProvider::availableTemplates();
    for (const auto &pair : templates)
        m_templateCombo->addItem(pair.second, pair.first);
    int templateIndex = m_templateCombo->findData(m_original.parserTemplate);
    if (templateIndex < 0)
        templateIndex = 0;
    m_templateCombo->setCurrentIndex(templateIndex);
    form->addRow(QStringLiteral("解析模板："), m_templateCombo);

    m_endpointEdit = new QLineEdit(m_original.endpoint);
    m_endpointEdit->setPlaceholderText(QStringLiteral("https://…"));
    form->addRow(QStringLiteral("访问 URL："), m_endpointEdit);

    m_keyEdit = new QLineEdit;
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setPlaceholderText(m_editMode
        ? QStringLiteral("留空 = 保持已保存的 Key 不变")
        : QStringLiteral("粘贴 API Key（保存到 macOS 钥匙串）"));
    form->addRow(QStringLiteral("API Key："), m_keyEdit);

    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(30, 3600);
    m_intervalSpin->setSuffix(QStringLiteral(" 秒"));
    m_intervalSpin->setValue(m_original.refreshIntervalSec > 0
                                 ? m_original.refreshIntervalSec : 120);
    form->addRow(QStringLiteral("刷新间隔："), m_intervalSpin);

    layout->addLayout(form);

    // custom_json 字段映射高级区
    m_customFieldsBox = new QGroupBox(QStringLiteral("字段映射（custom_json 专用）"));
    auto *fieldsForm = new QFormLayout(m_customFieldsBox);
    const QHash<QString, QString> hints = {
        {QStringLiteral("quotasArray"), QStringLiteral("配额数组路径，如 data.limits")},
        {QStringLiteral("used"), QStringLiteral("已用量键名，如 currentValue")},
        {QStringLiteral("limit"), QStringLiteral("上限键名，如 limitValue")},
        {QStringLiteral("reset"), QStringLiteral("重置时间键名（秒/毫秒 epoch）")},
        {QStringLiteral("balance"), QStringLiteral("余额路径，如 data.total_balance")},
        {QStringLiteral("currency"), QStringLiteral("币种路径（可空默认 CNY）")},
    };
    for (const char *key : kCustomFieldKeys) {
        const QString k = QString::fromLatin1(key);
        auto *edit = new QLineEdit(m_original.fields.value(k).toString());
        edit->setPlaceholderText(hints.value(k));
        fieldsForm->addRow(k + QStringLiteral("："), edit);
        m_fieldEdits.insert(k, edit);
    }
    layout->addWidget(m_customFieldsBox);

    const bool isCustom = m_templateCombo->currentData().toString()
                          == QStringLiteral("custom_json");
    m_customFieldsBox->setVisible(isCustom);

    connect(m_templateCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        const QString t = m_templateCombo->currentData().toString();
        m_customFieldsBox->setVisible(t == QStringLiteral("custom_json"));
        m_endpointEdit->setPlaceholderText(endpointPlaceholderFor(t));
        // 端点为空或仍是某模板默认值时，跟随模板预填
        const QString current = m_endpointEdit->text().trimmed();
        bool isKnownDefault = current.isEmpty();
        if (!isKnownDefault) {
            for (const auto &pair : HttpProvider::availableTemplates()) {
                if (current == defaultEndpointFor(pair.first)) {
                    isKnownDefault = true;
                    break;
                }
            }
        }
        if (isKnownDefault)
            m_endpointEdit->setText(defaultEndpointFor(t));
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(m_editMode
        ? QStringLiteral("保存") : QStringLiteral("添加"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &AddProviderDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    // 添加模式下端点先按默认模板预填
    if (!m_editMode) {
        m_endpointEdit->setPlaceholderText(
            endpointPlaceholderFor(m_templateCombo->currentData().toString()));
        if (m_endpointEdit->text().isEmpty())
            m_endpointEdit->setText(
                defaultEndpointFor(m_templateCombo->currentData().toString()));
    }
}

void AddProviderDialog::accept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少名称"),
                             QStringLiteral("请填写提供商名称。"));
        return;
    }
    if (m_endpointEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("缺少 URL"),
                             QStringLiteral("请填写接口访问 URL。"));
        return;
    }
    QDialog::accept();
}

ProviderConfig AddProviderDialog::config() const
{
    ProviderConfig config = m_original;
    if (!m_editMode) {
        config.id = QStringLiteral("user-%1")
                        .arg(QDateTime::currentMSecsSinceEpoch(), 0, 36);
        config.source = QStringLiteral("user");
        config.enabled = true;
    }
    config.name = m_nameEdit->text().trimmed();
    config.type = QStringLiteral("http");
    config.parserTemplate = m_templateCombo->currentData().toString();
    config.endpoint = m_endpointEdit->text().trimmed();
    config.authHeader = QStringLiteral("Authorization");
    config.authPrefix = QStringLiteral("Bearer ");
    config.credentialKey = QStringLiteral("apiKey");
    config.refreshIntervalSec = m_intervalSpin->value();

    const bool isCustom = config.parserTemplate == QStringLiteral("custom_json");
    config.supportsQuota = config.parserTemplate == QStringLiteral("quota_windows")
                           || config.parserTemplate == QStringLiteral("glm_quota")
                           || isCustom;
    config.supportsApiUsage = !config.supportsQuota || isCustom;

    if (isCustom) {
        QJsonObject fields;
        for (auto it = m_fieldEdits.constBegin(); it != m_fieldEdits.constEnd(); ++it) {
            const QString value = it.value()->text().trimmed();
            if (!value.isEmpty())
                fields.insert(it.key(), value);
        }
        config.fields = fields;
    }

    return config;
}

QString AddProviderDialog::apiKey() const
{
    return m_keyEdit->text().trimmed();
}
