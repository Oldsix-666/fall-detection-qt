#include "ui/mainwindow.h"
#include "ui/videowidget.h"
#include "detector/detectionworker.h"

// Qt 控件头文件
#include <QWidget>
#include <QVBoxLayout>       // 垂直布局
#include <QHBoxLayout>       // 水平布局
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QThread>
#include <QMetaObject>       // invokeMethod
#include <QDebug>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    buildUi();
}

MainWindow::~MainWindow()
{
    // 如果 worker 存在，先请求停止
    // 【同上】必须直接调用。用 QueuedConnection 的话，
    // 阻塞循环里的子线程永远不会执行它，这里就会白等满 3 秒，关窗卡顿
    if (m_worker)
        m_worker->stop();

    // 关闭线程
    if (m_thread) {
        m_thread->quit();       // 退出事件循环
        m_thread->wait(3000);   // 等待最多 3 秒
    }
}

void MainWindow::buildUi()
{
    // ========================================================
    // 【new QWidget(this)】创建一个 QWidget 作为中央部件
    // 【this】作为 parent，会被主窗口自动 delete
    // ========================================================
    auto* central = new QWidget(this);
    // 【setCentralWidget】QMainWindow 特有：设置中央区域
    setCentralWidget(central);

    // 【QHBoxLayout】水平布局：子控件从左到右排列
    auto* topBar = new QHBoxLayout;

    // 【addWidget(指针)】把控件加入布局
    topBar->addWidget(new QLabel(QStringLiteral("视频源:")));

    m_sourceBox = new QComboBox;
    // ========================================================
    // 【addItem(显示文本, 关联数据)】
    // 显示 "摄像头" 给用户看，实际存的是枚举值
    // ========================================================
    m_sourceBox->addItem(QStringLiteral("摄像头"), SourceConfig::Camera);
    m_sourceBox->addItem(QStringLiteral("RTSP"),   SourceConfig::Rtsp);
    topBar->addWidget(m_sourceBox);

    // ========================================================
    // 【QStackedWidget】堆叠控件：多个页面，同时只显示一个
    // 通过 setCurrentIndex 切换
    // ========================================================
    m_sourceStack = new QStackedWidget;

    // ============ 摄像头参数页 ============
    auto* camPage = new QWidget;
    auto* camL = new QHBoxLayout(camPage);
    // 【setContentsMargins】设置布局四周留白
    // (左, 上, 右, 下) 单位像素
    camL->setContentsMargins(0, 0, 0, 0);
    camL->addWidget(new QLabel(QStringLiteral("索引:")));

    m_camBox = new QComboBox;
    // 【for 循环】i 从 0 到 4
    for (int i = 0; i < 5; ++i)
        // 【QString::number(i)】int → QString
        m_camBox->addItem(QString::number(i), i);
    camL->addWidget(m_camBox);
    m_sourceStack->addWidget(camPage);   // 加入堆叠，索引 0

    // ============ RTSP 参数页 ============
    auto* rtspPage = new QWidget;
    auto* rtspL = new QHBoxLayout(rtspPage);
    rtspL->setContentsMargins(0, 0, 0, 0);
    rtspL->addWidget(new QLabel(QStringLiteral("URL:")));

    m_rtspEdit = new QLineEdit;
    // 【setPlaceholderText】灰色提示文字（用户输入会覆盖）
    m_rtspEdit->setPlaceholderText(
        QStringLiteral("rtsp://admin:password@192.168.1.100:554/Streaming/Channels/101"));
    m_rtspEdit->setMinimumWidth(400);
    rtspL->addWidget(m_rtspEdit);
    m_sourceStack->addWidget(rtspPage);   // 索引 1

    topBar->addWidget(m_sourceStack);

    // ============ 按钮 ============
    m_startBtn = new QPushButton(QStringLiteral("开始检测"));
    m_stopBtn  = new QPushButton(QStringLiteral("停止"));
    m_stopBtn->setEnabled(false);   // 【setEnabled(false)】禁用按钮
    topBar->addWidget(m_startBtn);
    topBar->addWidget(m_stopBtn);
    // 【addStretch()】弹簧：把后续内容推到最右边
    topBar->addStretch();

    // ============ 状态显示 ============
    m_statusLabel = new QLabel(QStringLiteral("就绪"));
    m_fpsLabel    = new QLabel(QStringLiteral("FPS: --"));
    topBar->addWidget(m_statusLabel);
    topBar->addWidget(m_fpsLabel);

    // ============ 视频区 ============
    m_video = new VideoWidget(this);

    // 【QVBoxLayout】垂直布局：子控件从上到下排列
    auto* root = new QVBoxLayout(central);
    root->addLayout(topBar);
    // ========================================================
    // 【addWidget(widget, stretch)】
    //   stretch=1 表示这个控件占据所有剩余空间
    // ========================================================
    root->addWidget(m_video, 1);

    setWindowTitle(QStringLiteral("摔倒检测 - Fall Detection"));

    // ============ 信号槽连接 ============
    // ========================================================
    // 【connect(sender, signal, receiver, slot)】
    // 【&类::成员】C++11 成员函数指针
    // Qt5 起推荐这种语法，编译期检查参数匹配
    // ========================================================
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_stopBtn,  &QPushButton::clicked, this, &MainWindow::onStopClicked);

    // ========================================================
    // 【QOverload<int>::of】用于区分同名不同参数的重载函数
    // 这里因为 currentIndexChanged 有 int 和 QString 两个重载
    // ========================================================
    connect(m_sourceBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSourceTypeChanged);

    onSourceTypeChanged(0);   // 主动调用一次，初始化显示
}

void MainWindow::onSourceTypeChanged(int)
{
    // ========================================================
    // 【currentData()】获取当前选中项的关联数据（addItem 的第二个参数）
    // 【toInt()】QVariant → int
    // ========================================================
    const int type = m_sourceBox->currentData().toInt();

    // 三元：根据类型切换页
    m_sourceStack->setCurrentIndex(type == SourceConfig::Camera ? 0 : 1);
}

SourceConfig MainWindow::currentSourceConfig() const
{
    SourceConfig cfg;
    const int type = m_sourceBox->currentData().toInt();

    if (type == SourceConfig::Camera) {
        cfg.type        = SourceConfig::Camera;
        cfg.cameraIndex = m_camBox->currentData().toInt();
    } else {
        cfg.type    = SourceConfig::Rtsp;
        // 【text()】获取输入框内容
        // 【trimmed()】去掉首尾空格
        cfg.rtspUrl = m_rtspEdit->text().trimmed();
    }
    return cfg;
}

void MainWindow::onStartClicked()
{
    // 【m_thread】非空表示已在运行
    if (m_thread) return;

    const SourceConfig cfg = currentSourceConfig();

    if (cfg.type == SourceConfig::Rtsp && cfg.rtspUrl.isEmpty()) {
        // ====================================================
        // 【setText】设置标签文字
        // 支持简单 HTML：<b>加粗</b>，style='color:red' 变红
        // ====================================================
        m_statusLabel->setText(QStringLiteral("<b style='color:red'>请填写 RTSP 地址</b>"));
        return;
    }

    // ============ 创建线程和 worker ============
    // ========================================================
    // 【new QThread(this)】parent 是 this，
    // 会在 this 析构时自动 delete，避免泄漏
    // ========================================================
    m_thread = new QThread(this);
    m_worker = new DetectionWorker();

    // ========================================================
    // 【moveToThread】把 worker 的"逻辑归属"移到子线程
    // 效果：worker 的所有 slot 都会在子线程的事件循环中被调用
    // ========================================================
    m_worker->moveToThread(m_thread);

    // 线程结束后自动 delete worker（避免泄漏）
    // 【QThread::finished】是 QThread 的信号
    // 【&QObject::deleteLater】Qt 的延迟删除槽
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    // worker → 主窗口的信号（跨线程自动队列化）
    connect(m_worker, &DetectionWorker::frameReady,    this, &MainWindow::onFrameReady);
    connect(m_worker, &DetectionWorker::errorOccurred, this, &MainWindow::onError);
    connect(m_worker, &DetectionWorker::started,       this, &MainWindow::onWorkerStarted);
    connect(m_worker, &DetectionWorker::stopped,       this, &MainWindow::onWorkerStopped);

    // 启动线程（进入子线程事件循环）
    m_thread->start();

    // ============ 异步调用 worker 的 start ============
    // ========================================================
    // 【QMetaObject::invokeMethod(obj, "方法名", 连接类型, 参数...)]
    // 按函数名字符串调用（不走 C++ 编译检查，靠元系统运行期查找）
    // 【Qt::QueuedConnection】投递到目标对象所属线程的事件队列
    // 【Q_ARG(类型, 值)】封装参数，需要类型已注册元类型
    // ========================================================
    QMetaObject::invokeMethod(m_worker, "start",
                              Qt::QueuedConnection,
                              Q_ARG(SourceConfig, cfg));
}

void MainWindow::onStopClicked()
{
    // ========================================================
    // 【关键】这里必须"直接调用"，绝对不能用 Qt::QueuedConnection！
    //
    // DetectionWorker::start() 在子线程里是个阻塞的 while 循环，
    // 一旦跑起来，子线程的事件循环就停止运转了。
    // 如果用 QueuedConnection，"stop" 这个调用只会被排进子线程的
    // 事件队列，而子线程永远没机会处理它
    //   —— 现象就是：点"停止"按钮完全没反应。
    //
    // stop() 内部只写一个 std::atomic_bool，本身是线程安全的，
    // 从主线程直接调用没有任何问题。
    // ========================================================
    if (m_worker)
        m_worker->stop();

    m_stopBtn->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("正在停止..."));
}

void MainWindow::onFrameReady(const QImage& frame, double fps, bool fall)
{
    m_video->setFrame(frame);
    // ========================================================
    // 【arg(值, 字段宽度, 格式, 精度)】
    // 'f' 表示浮点数，1 表示 1 位小数
    // ========================================================
    m_fpsLabel->setText(QStringLiteral("FPS: %1").arg(fps, 0, 'f', 1));

    if (fall) {
        m_statusLabel->setText(QStringLiteral("<b style='color:red'>⚠ 检测到摔倒！</b>"));
    } else {
        m_statusLabel->setText(QStringLiteral("检测中..."));
    }
}

void MainWindow::onError(const QString& msg)
{
    m_statusLabel->setText(QStringLiteral("<b style='color:red'>错误: %1</b>").arg(msg));
    qDebug() << "[MainWindow] 错误:" << msg;
}

void MainWindow::onWorkerStarted()
{
    // 运行中：禁用开始和源选择
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_sourceBox->setEnabled(false);
    m_sourceStack->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("检测中..."));
}

void MainWindow::onWorkerStopped()
{
    // 优雅关闭线程
    if (m_thread) {
        m_thread->quit();          // 请求退出事件循环
        m_thread->wait(2000);      // 最多等 2 秒
        // ====================================================
        // 【deleteLater】延迟删除（等当前事件处理完）
        // 不能在信号槽里直接 delete，可能触发重入问题
        // ====================================================
        m_thread->deleteLater();
        m_thread = nullptr;
        m_worker = nullptr;
    }

    // 恢复 UI
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_sourceBox->setEnabled(true);
    m_sourceStack->setEnabled(true);
    m_video->clear();
    m_fpsLabel->setText(QStringLiteral("FPS: --"));
    m_statusLabel->setText(QStringLiteral("就绪"));
}