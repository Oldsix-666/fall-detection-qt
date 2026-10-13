#include "ui/videowidget.h"
#include <QPainter>   // 绘图类

// ============================================================
// 【初始化列表 : QWidget(parent)】
// 调用父类 QWidget 的构造函数
// 传入 parent 后，Qt 会在 parent 析构时自动删除本控件
// （对象树机制）
// ============================================================
VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent)
{
    // 最小尺寸
    setMinimumSize(640, 480);

    // ========================================================
    // 【setAttribute(Qt::WA_OpaquePaintEvent)】
    // 告诉 Qt：本控件不透明，且 paintEvent 里会自己填满整个区域
    // 优化：Qt 就不必再帮我们擦一遍背景了
    //       （paintEvent 开头已经 fillRect(rect(), Qt::black)）
    //
    // 【注意】不要在这里再调用 setAutoFillBackground(true)！
    // 那会让 Qt 又填一次背景，和上面的优化正好互相抵消，
    // 等于每帧白白多刷一遍。
    // ========================================================
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void VideoWidget::setFrame(const QImage& img)
{
    m_frame = img;
    // ========================================================
    // 【update()】请求重绘
    // 是异步的：只是标记需要重绘，实际 paintEvent 由事件循环调用
    // ========================================================
    update();
}

void VideoWidget::clear()
{
    // 【QImage()】默认构造：空图像
    m_frame = QImage();
    update();
}

void VideoWidget::paintEvent(QPaintEvent*)
{
    // ========================================================
    // 【QPainter p(this)】构造画笔，绑定到 this 控件
    // 作用域结束时自动析构（RAII：资源获取即初始化）
    // ========================================================
    QPainter p(this);

    // 【fillRect(rect(), color)】把整个控件涂黑
    // 【rect()】返回控件内矩形
    p.fillRect(rect(), Qt::black);

    // 【isNull()】判断 QImage 是否为空
    if (m_frame.isNull()) {
        p.setPen(Qt::gray);
        // ====================================================
        // 【drawText(矩形, 对齐方式, 字符串)】
        // 【Qt::AlignCenter】水平垂直居中
        // 【QStringLiteral】编译期生成 QString，比 QString("...") 快
        // ====================================================
        p.drawText(rect(), Qt::AlignCenter,
                   QStringLiteral("点击「开始检测」启动视频源"));
        return;
    }

    // 等比缩放算法：
    // 【QSize sz = m_frame.size()】拷贝原图尺寸
    QSize sz = m_frame.size();
    // ========================================================
    // 【scale(目标尺寸, 模式)】
    // 【Qt::KeepAspectRatio】保持纵横比
    // 只缩放到能放进目标尺寸
    // ========================================================
    sz.scale(size(), Qt::KeepAspectRatio);

    // 【QRect(QPoint(0,0), size)】左上角在原点
    QRect target(QPoint(0, 0), sz);
    // 【moveCenter】保持尺寸不变，移动使中心对齐
    target.moveCenter(rect().center());

    // ========================================================
    // 【drawImage(目标矩形, 源图像)】
    // 会自动缩放源图像到目标矩形大小
    // ========================================================
    p.drawImage(target, m_frame);
}