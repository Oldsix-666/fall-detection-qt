#pragma once

#include <QString>
#include <memory>
#include <opencv2/core.hpp>   // cv::Mat

// ============================================================
// IFrameSource：视频帧源的抽象接口（纯虚基类）
// CameraSource / RtspSource 都实现它，
// 上层只用 IFrameSource* 就能操作任意类型的源（多态）
// ============================================================
class IFrameSource
{
public:
    // 【虚析构】基类指针 delete 派生类对象时必须为 virtual，
    // 否则派生类的析构函数不会被调用（资源泄漏）
    virtual ~IFrameSource() = default;

    // 打开源；失败时把原因写入 err（err 允许为 nullptr）
    virtual bool open(QString* err = nullptr) = 0;

    // 读取一帧 BGR 图像；成功返回 true
    virtual bool read(cv::Mat& frame) = 0;

    // 释放资源（要求可重复调用）
    virtual void close() = 0;

    // 用于日志 / 界面显示的源名称
    virtual QString name() const = 0;
};

// 【using 别名】所有权的统一类型：
// 工厂 create() 返回它，调用方独占持有，出作用域自动析构
using IFrameSourcePtr = std::unique_ptr<IFrameSource>;
