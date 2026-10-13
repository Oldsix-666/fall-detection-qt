#pragma once

#include <QWidget>
#include <QImage>

// ============================================================
// VideoWidget：显示视频帧的控件
// 继承 QWidget，重写 paintEvent 自定义绘制
// ============================================================
class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    // 【explicit】禁止隐式转换
    // QWidget* parent 默认 nullptr：可以独立创建
    explicit VideoWidget(QWidget* parent = nullptr);

    // 【slot】可被信号连接的函数
public slots:
    void setFrame(const QImage& img);
    void clear();

protected:
    // ========================================================
    // 【override】重写 QWidget::paintEvent
    // 参数 QPaintEvent* 事件对象，通常不用
    //   实现里可省略参数名：void paintEvent(QPaintEvent*)
    // ========================================================
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_frame;
};