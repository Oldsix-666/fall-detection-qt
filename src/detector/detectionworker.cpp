#include "detector/detectionworker.h"
#include "source/framesourcefactory.h"

#include <opencv2/opencv.hpp>     // 常用模块（注意：OpenCV 5.0 的 opencv.hpp 不再包含 geometry）
#include <opencv2/geometry.hpp>   // OpenCV 5.0：contourArea / boundingRect 被移到了 geometry 模块
#include <QThread>              // QThread::msleep
#include <QElapsedTimer>        // 高精度计时
#include <QDebug>

DetectionWorker::DetectionWorker(QObject* parent) : QObject(parent) {}
DetectionWorker::~DetectionWorker() = default;

// ============================================================
// 局部小工具：保证 start() 无论从哪条 return 路径退出，
// m_running 都会被复位为 false（RAII：构造时置位，析构时清零）
// ============================================================
namespace {
struct RunningGuard {
    std::atomic_bool& flag;
    explicit RunningGuard(std::atomic_bool& f) : flag(f) { flag = true; }
    ~RunningGuard() { flag = false; }
};
}   // namespace

// ============================================================
// start：在子线程中执行的主循环
// 注意：这个函数会一直阻塞，直到 m_running 变为 false
// ============================================================
void DetectionWorker::start(SourceConfig cfg)
{
    // 【guard】进入即 m_running = true；
    // 之后不管是正常跑完、读帧失败 break，还是中途 return，
    // 离开作用域时都会自动复位为 false，不会"卡在 true"。
    RunningGuard guard(m_running);

    m_judge.reset();        // 清空上次运行残留历史
    emit started();         // 【emit】触发信号（宏，展开为空，纯语法糖）

    // ========================================================
    // 工厂创建源（多态：
    //   无论 Camera 还是 Rtsp，都用 IFrameSource 接口操作）
    // ========================================================
    IFrameSourcePtr src = FrameSourceFactory::create(cfg);
    if (!src) {
        emit errorOccurred("视频源创建失败");
        emit stopped();
        return;             // 提前退出
    }

    QString err;
    // 【&err】取地址传给 open 的 QString* 参数
    if (!src->open(&err)) {
        emit errorOccurred(err);
        emit stopped();
        return;
    }

    qDebug() << "[Worker] 视频源启动:" << src->name();

    // ========================================================
    // 【cv::Ptr<T>】OpenCV 的智能指针（类似 std::shared_ptr）
    // 【createBackgroundSubtractorMOG2】返回这个类型的指针
    // 参数：
    //   500      - 历史帧数（越大越稳定，适应光线变化越慢）
    //   16       - 方差阈值
    //   false    - 不检测阴影
    // 作用：把"运动的前景"从"静止的背景"中分离
    // ========================================================
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog =
        cv::createBackgroundSubtractorMOG2(500, 16, false);

    // 预分配 Mat 对象，循环里复用（避免反复分配）
    cv::Mat frame;      // 原始 BGR 帧
    cv::Mat fg;         // 前景掩码（单通道 8UC1）
    cv::Mat rgb;        // RGB 缓冲
    cv::Mat small;      // 降采样小图

    QElapsedTimer timer;
    timer.start();      // 开始计时

    int    frames     = 0;
    double fps        = 0.0;
    int    emptyCount = 0;   // 连续读帧失败计数

    // ============ 主循环 ============
    // 【load()】原子读，返回当前值
    while (m_running.load()) {

        // 读帧失败：累积计数，超过阈值认为流已断开
        if (!src->read(frame)) {
            // 【++emptyCount】前置自增
            // 【> 60】连续失败超过 60 次（约 1 秒）就退出
            if (++emptyCount > 60) {
                emit errorOccurred("连续读帧失败，视频源断开");
                break;   // 跳出 while 循环
            }
            // 【QThread::msleep(ms)】静态函数，让当前线程休眠
            // 避免读失败时疯狂空转吃满 CPU
            QThread::msleep(5);
            continue;   // 跳过本次循环剩余代码
        }
        emptyCount = 0;

        // ---- 步骤1：降采样 ----
        // 【cv::resize(输入, 输出, 目标尺寸)】
        // 【cv::Size(320, 240)】构造尺寸对象
        cv::resize(frame, small, cv::Size(320, 240));

        // ---- 步骤2：应用背景建模 ----
        // 【apply(输入, 输出掩码)】
        // 第一次调用会初始化背景模型，前几帧效果较差，属正常
        mog->apply(small, fg);

        // ---- 步骤3：形态学开运算去噪 ----
        // 【morphologyEx(src, dst, op, kernel)】
        //   op = MORPH_OPEN 表示先腐蚀后膨胀
        // 【getStructuringElement(shape, size)】
        //   创建 5x5 的椭圆结构元素
        //   {5, 5} 是 C++11 列表初始化，等价于 cv::Size(5, 5)
        cv::morphologyEx(fg, fg, cv::MORPH_OPEN,
                         cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5}));

        // ---- 步骤4：找轮廓 ----
        // 【std::vector<std::vector<cv::Point>>】嵌套容器
        //   外层：每个轮廓
        //   内层：轮廓上的点
        std::vector<std::vector<cv::Point>> contours;
        // 【RETR_EXTERNAL】只找最外层轮廓
        // 【CHAIN_APPROX_SIMPLE】只存拐点，节省内存
        cv::findContours(fg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double   maxArea = 0.0;
        cv::Rect bestBox;
        // 【const auto&】范围 for 遍历，用引用避免拷贝
        for (const auto& c : contours) {
            // 【contourArea】计算轮廓面积
            double a = cv::contourArea(c);
            if (a > maxArea) {
                maxArea = a;
                // 【boundingRect】返回轮廓的最小正矩形
                bestBox = cv::boundingRect(c);
            }
        }

        // ---- 步骤5：坐标映射回原图 ----
        // 计算缩放比例。float(...) 是 C++ 显式转换
        const float sx = float(frame.cols) / 320.f;
        const float sy = float(frame.rows) / 240.f;

        cv::Rect dispBox;
        if (maxArea > 0) {
            // 【显式构造 cv::Rect(x, y, w, h)】
            dispBox = cv::Rect(int(bestBox.x * sx), int(bestBox.y * sy),
                               int(bestBox.width * sx), int(bestBox.height * sy));
        }

        // ---- 步骤6：调用判定 ----
        // 【QRectF(x, y, w, h)】浮点矩形
        QRectF qbox(dispBox.x, dispBox.y, dispBox.width, dispBox.height);
        // 面积也要按比例还原（线性缩放，面积按平方）
        const double realArea = maxArea * sx * sy;
        const bool fallAlarm = m_judge.update(qbox, realArea);

        // ---- 画框 ----
        if (m_judge.lastValid()) {
            // 【三元运算符】条件 ? 值1 : 值2
            // 【cv::Scalar(B, G, R)】BGR 顺序（OpenCV 特有！）
            cv::Scalar color = fallAlarm ? cv::Scalar(0, 0, 255)
                                         : cv::Scalar(0, 255, 0);
            // 【rectangle(图像, 矩形, 颜色, 线宽)】
            cv::rectangle(frame, dispBox, color, 2);

            // 【snprintf】安全格式化到字符数组
            // 【std::max(a,b)】取较大值
            char buf[64];
            std::snprintf(buf, sizeof(buf), "aspect=%.2f", m_judge.lastAspect());
            // 【putText】在图像上写文字
            cv::putText(frame, buf,
                        cv::Point(dispBox.x, std::max(20, dispBox.y - 8)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }

        // ---- 摔倒报警横幅 ----
        if (fallAlarm) {
            // 【cv::FILLED】填充标志（-1 也代表填充）
            cv::rectangle(frame, cv::Rect(0, frame.rows - 70, frame.cols, 70),
                          cv::Scalar(0, 0, 200), cv::FILLED);
            cv::putText(frame, "FALL DETECTED",
                        cv::Point(20, frame.rows - 22),
                        cv::FONT_HERSHEY_SIMPLEX, 1.2,
                        cv::Scalar(255, 255, 255), 3);
        }

        // ---- FPS 显示 ----
        if (fps > 0) {
            char fpsBuf[32];
            std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", fps);
            cv::putText(frame, fpsBuf,
                        cv::Point(frame.cols - 130, 30),
                        cv::FONT_HERSHEY_SIMPLEX, 0.7,
                        cv::Scalar(0, 255, 255), 2);
        }

        // ---- 转换颜色空间 ----
        // 【cvtColor(src, dst, code)】
        // 【COLOR_BGR2RGB】BGR -> RGB
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

        // ====================================================
        // ---- 构造 QImage ----
        // 【QImage(data, width, height, bytesPerLine, format)】
        // 注意：这只共享 rgb 的数据，不拷贝
        // 【int(rgb.step)】step 是每行字节数（可能带内存对齐填充）
        // ====================================================
        QImage img(rgb.data, rgb.cols, rgb.rows,
                   int(rgb.step), QImage::Format_RGB888);

        // 【关键】深拷贝！
        // 避免下一轮循环 rgb 被覆盖，导致 QImage 变成野指针
        img = img.copy();

        // ---- FPS 统计 ----
        ++frames;
        // 【elapsed()】返回毫秒
        if (timer.elapsed() >= 1000) {
            // 【1000.0】必须是浮点，否则整数除法结果为 0
            fps = frames * 1000.0 / timer.elapsed();
            frames = 0;
            // 【restart()】重置计时器并重新开始
            timer.restart();
        }

        // 发信号（跨线程自动队列化到主线程）
        emit frameReady(img, fps, fallAlarm);
    }
    // ============ 循环结束 ============

    src->close();
    emit stopped();   // 通知主线程
}

void DetectionWorker::stop()
{
    // 只有这一行！因为 m_running 是原子的，
    // 循环中的 load() 会看到最新值并退出
    m_running = false;
}