#include "source/camerasource.h"
#include <QDebug>   // qDebug() << 调试输出

// ============================================================
// 【构造函数】成员初始化列表语法
//   : m_index(index), m_width(width), m_height(height)
// 好处：直接初始化成员（不是先默认构造再赋值），效率更高
// 顺序：由成员声明顺序决定，不是初始化列表顺序
// ============================================================
CameraSource::CameraSource(int index, int width, int height)
    : m_index(index), m_width(width), m_height(height)
{
    // 构造函数体为空，参数已通过初始化列表处理
}

// 【析构函数】对象销毁时自动调用
// 调用 close() 保证资源被释放
CameraSource::~CameraSource() { close(); }

// ============================================================
// open：打开摄像头
// ============================================================
bool CameraSource::open(QString* err)
{
    // 【#ifdef _WIN32】预处理条件编译
    //   MSVC 会定义 _WIN32 宏
    //   同一份代码适配不同平台
#ifdef _WIN32
    // 【cv::CAP_DSHOW】指定 DirectShow 后端
    //   Windows 上打开摄像头比默认 MSMF 更快
    m_cap.open(m_index, cv::CAP_DSHOW);
#else
    // Linux 下用默认后端（通常是 V4L2）
    m_cap.open(m_index);
#endif

    // 【isOpened()】查询是否成功打开
    if (!m_cap.isOpened()) {
        // 【if (err)】指针判空：只有调用方传了有效指针才写
        // 【QString("...%1...").arg(m_index)】
        //   %1 会被第一个 arg() 参数替换
        if (err) *err = QString("无法打开摄像头 %1，检查是否被占用").arg(m_index);
        return false;
    }

    // 【set(prop, value)】设置摄像头参数
    // 部分驱动会忽略或调整到最近支持值
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH,  m_width);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, m_height);
    m_cap.set(cv::CAP_PROP_FPS, 30);

    // ========================================================
    // 【关键】缓冲设为 1
    // 默认缓冲有 5~10 帧，会累积延迟
    // 设成 1 后读到的永远是"最新一帧"
    // ========================================================
    m_cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // 【qDebug()】Qt 调试输出宏
    // << 运算符可串联多个值，自动加空格
    qDebug() << "[CameraSource] index=" << m_index << "分辨率"
             << m_cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
             << m_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    return true;
}

bool CameraSource::read(cv::Mat& frame)
{
    // 未打开直接失败
    if (!m_cap.isOpened()) return false;

    // 【read(frame)】读一帧并返回是否成功
    // 【&& !frame.empty()】双重保险：有时 read 返回 true 但帧为空
    return m_cap.read(frame) && !frame.empty();
}

void CameraSource::close()
{
    // 只有打开着才需要 release，避免重复释放
    if (m_cap.isOpened()) m_cap.release();
}

QString CameraSource::name() const
{
    // 【QString("Camera %1").arg(m_index)】格式化，如 "Camera 0"
    return QString("Camera %1").arg(m_index);
}