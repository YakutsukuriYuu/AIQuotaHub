#pragma once

#include <QWidget>

// 圆环进度控件：QPainter 自绘圆环弧 + 中心百分比 + 环内小字
class RingProgress : public QWidget
{
    Q_OBJECT
public:
    explicit RingProgress(QWidget *parent = nullptr);

    void setValue(double percent);          // 0~1；负值 = 无数据（灰环）
    void setSubText(const QString &text);   // 环内下方小字（如 "32/80"）

    QSize sizeHint() const override { return {92, 92}; }
    QSize minimumSizeHint() const override { return {72, 72}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_percent = -1;
    QString m_subText;
};
