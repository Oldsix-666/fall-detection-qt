#pragma once

#include "source/iframesource.h"
#include <opencv2/videoio.hpp>

// RTSP 源，结构类似 CameraSource
class RtspSource : public IFrameSource
{
public:
    // ========================================================
    // 【explicit】关键字：禁止隐式类型转换
    // 例如：RtspSource s = "rtsp://...";  // 编译错误
    //       RtspSource s("rtsp://...");   // 正确
    // 避免意外把 QString 转成 RtspSource 对象
    // ========================================================
    explicit RtspSource(const QString& url);

    ~RtspSource() override;

    bool    open(QString* err = nullptr) override;
    bool    read(cv::Mat& frame) override;
    void    close() override;
    QString name() const override;

private:
    QString          m_url;   // RTSP 地址
    cv::VideoCapture m_cap;   // OpenCV 句柄
};