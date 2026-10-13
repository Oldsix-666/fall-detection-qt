#pragma once

#include <QMainWindow>
#include "core/types.h"

// ============================================================
// 【前置声明】
// 只声明"有这么一个类"，不 include 对应头文件
// 好处：
//   1. 加快编译（不需要展开头文件）
//   2. 打破循环依赖
// 要求：只在这个类里用指针/引用，不用它的完整定义
// ============================================================
class VideoWidget;
class DetectionWorker;
class QThread;
class QPushButton;
class QComboBox;
class QLineEdit;
class QLabel;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;   // 声明析构，需要在 .cpp 里定义

    // 【private slots:】只有本类内部能连接的槽
private slots:
    void onStartClicked();
    void onStopClicked();
    void onSourceTypeChanged(int index);
    void onFrameReady(const QImage& frame, double fps, bool fall);
    void onError(const QString& msg);
    void onWorkerStarted();
    void onWorkerStopped();

private:
    void buildUi();
    SourceConfig currentSourceConfig() const;

    // ========================================================
    // 【= nullptr】C++11 起支持类内初始化成员
    // 所有指针都初始化为空，避免野指针
    // ========================================================
    VideoWidget*    m_video       = nullptr;
    QPushButton*    m_startBtn    = nullptr;
    QPushButton*    m_stopBtn     = nullptr;
    QComboBox*      m_sourceBox   = nullptr;
    QStackedWidget* m_sourceStack = nullptr;
    QComboBox*      m_camBox      = nullptr;
    QLineEdit*      m_rtspEdit    = nullptr;
    QLabel*         m_fpsLabel    = nullptr;
    QLabel*         m_statusLabel = nullptr;

    QThread*         m_thread = nullptr;
    DetectionWorker* m_worker = nullptr;
};