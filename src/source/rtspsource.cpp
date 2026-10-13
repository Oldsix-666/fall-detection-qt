#include "source/rtspsource.h"
#include <QDebug>

// 【构造函数】只有 m_url 一个参数
RtspSource::RtspSource(const QString& url) : m_url(url) {}
RtspSource::~RtspSource() { close(); }

bool RtspSource::open(QString* err)
{
    // ========================================================
    // 【关键】RTSP 默认走 UDP，丢包会造成画面花屏/绿块
    // 通过环境变量强制 FFmpeg 用 TCP 传输
    // 必须在第一次 open() 之前设置才能生效
    // ========================================================
#ifdef _WIN32
    // 【_putenv_s】MSVC 提供的安全版 setenv
    // 参数：变量名、值
    _putenv_s("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp");
#else
    // 【setenv】POSIX 版本
    // 参数：变量名、值、是否覆盖已有值（1 表示覆盖）
    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp", 1);
#endif

    // 【toStdString()】QString → std::string
    // 【cv::CAP_FFMPEG】FFmpeg 后端，读网络流必备
    // ========================================================
    // 【关键：超时设置】这两个属性不能通过 set() 设置，
    // 只能作为 params 数组在 open() 时传入（OpenCV 4.5+ 支持）。
    // 不设的话：地址打不通时 open() 可能卡很久；
    // 断流时 read() 也会阻塞，导致"停止"按钮反应迟钝。
    // ========================================================
    std::vector<int> params = {
        cv::CAP_PROP_OPEN_TIMEOUT_MSEC, 5000,   // 连接超时 5 秒
        cv::CAP_PROP_READ_TIMEOUT_MSEC, 2000    // 读帧超时 2 秒
    };
    m_cap.open(m_url.toStdString(), cv::CAP_FFMPEG, params);

    if (!m_cap.isOpened()) {
        // 【相邻字符串】C++ 会自动拼接
        // 【\n】换行符
        if (err) *err = QString(
                       "无法打开 RTSP: %1\n排查:\n"
                       "1. ping 通摄像头 IP 吗？\n"
                       "2. 用户名密码对吗？\n"
                       "3. VLC 能打开这个地址吗？").arg(m_url);
        return false;
    }

    // 缓冲 1，防止延迟累积
    m_cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    qDebug() << "[RtspSource] 已连接:" << m_url;
    return true;
}

bool RtspSource::read(cv::Mat& frame)
{
    if (!m_cap.isOpened()) return false;
    return m_cap.read(frame) && !frame.empty();
}

void RtspSource::close()
{
    if (m_cap.isOpened()) m_cap.release();
}

QString RtspSource::name() const
{
    return "RTSP";
}