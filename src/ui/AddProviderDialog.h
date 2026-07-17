#pragma once

#include "../core/ProvidersConfig.h"

#include <QDialog>
#include <QHash>

class QLineEdit;
class QComboBox;
class QSpinBox;

// 添加 / 编辑自定义提供商：名称 + 解析模板 + URL + API Key + 刷新间隔，
// custom_json 模板时展开字段映射高级区
class AddProviderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddProviderDialog(QWidget *parent = nullptr);              // 添加模式
    AddProviderDialog(const ProviderConfig &existing, QWidget *parent); // 编辑模式

    ProviderConfig config() const;
    QString apiKey() const;   // 新输入的 Key（空 = 未修改）

protected:
    void accept() override;

private:
    void buildUi();

    ProviderConfig m_original;   // 编辑模式原配置（保留 id/source/enabled）
    bool m_editMode = false;

    QLineEdit *m_nameEdit = nullptr;
    QComboBox *m_templateCombo = nullptr;
    QLineEdit *m_endpointEdit = nullptr;
    QLineEdit *m_keyEdit = nullptr;
    QSpinBox *m_intervalSpin = nullptr;
    QWidget *m_customFieldsBox = nullptr;
    QHash<QString, QLineEdit *> m_fieldEdits;   // custom_json 字段映射
};
