#pragma once

#include <QObject>
#include <QImage>
#include <atomic>     // std::atomic，原子类型
#include "core/types.h"
#include "detector/falljudge.h"

// ============================================================
// DetectionWorker：在子线程中工作的对象
// 使用方式：
//   worker = new DetectionWorker;
//   worker->moveToThread(thread);
//   QMetaObject::invokeMethod(worker, "start", ...);
// ============================================================
class DetectionWorker : public QObject
{
    // ========================================================
    // 【Q_OBJECT 宏】
    // 必须放在类声明的最前（private 区默认）
    // 作用：让 Qt 的 moc 工具为这个类生成元对象代码
    //       从而支持信号槽、属性、qobject_cast 等
    // 没有它 signals/slots 关键字不生效
    // ========================================================
    Q_OBJECT

public:
    explicit DetectionWorker(QObject* parent = nullptr);
    ~DetectionWorker() override;

    // ========================================================
    // 【public slots:】Qt 关键字
    // 声明为 slot 的函数可以：
    //   1. 与信号 connect
    //   2. 通过 QMetaObject::invokeMethod 按名字调用
    // 底层就是普通函数，加了个"可被元系统调用"的标记
    // ========================================================
public slots:
    // 【按值传参 SourceConfig】
    // 跨线程调用必须是值（Qt 会拷贝到事件队列）
    void start(SourceConfig cfg);

    // ========================================================
    // 【重要】stop() 只写一个 std::atomic_bool，是线程安全的，
    // 调用方必须"直接调用"（或 DirectConnection），
    // 不能用 QueuedConnection —— start() 是阻塞循环，
    // 子线程事件循环不运转，排队的 stop 调用永远得不到执行。
    // ========================================================
    void stop();

    // ========================================================
    // 【signals:】Qt 关键字
    // 声明信号函数。信号只有声明没有实现（moc 自动生成）
    // 通过 emit signalName(args) 触发
    // ========================================================
signals:
    void frameReady(const QImage& frame, double fps, bool fall);
    void errorOccurred(const QString& msg);
    void started();
    void stopped();

private:
    // ========================================================
    // 【std::atomic<bool>】原子类型
    // 多线程读写不会出现数据竞争
    // 不需要额外加锁，性能比 mutex 好
    // 【{false}】C++11 统一初始化，等价于 m_running(false)
    // ========================================================
    std::atomic_bool m_running{false};

    FallJudge        m_judge;
};