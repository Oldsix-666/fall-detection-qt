#pragma once

// 【""包含】优先从 INCLUDEPATH 搜索，因此可以写 "source/xxx.h"
#include "source/iframesource.h"
#include <opencv2/videoio.hpp>   // cv::VideoCapture

// 【: public IFrameSource】公有继承
// CameraSource 是 IFrameSource 的一种，可以当 IFrameSource 用
class CameraSource : public IFrameSource
{
public:
    // 构造函数：创建对象时调用
    // 默认参数让调用方可以只传 index
    CameraSource(int index, int width = 1280, int height = 720);

    // 【override】C++11 关键字
    // 作用：显式声明"我在重写基类虚函数"。
    //   1. 编译时校验基类是否有对应虚函数，名字写错会立即报错
    //   2. 提高可读性
    // 有 override 后不需要再写 virtual（默认就是 virtual）
    ~CameraSource() override;

    bool    open(QString* err = nullptr) override;
    bool    read(cv::Mat& frame) override;
    void    close() override;
    QString name() const override;

private:
    int              m_index;    // 【m_ 前缀】：成员变量命名习惯
    int              m_width;
    int              m_height;
    cv::VideoCapture m_cap;      // OpenCV 摄像头句柄
};