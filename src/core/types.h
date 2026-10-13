#pragma once
#include <QString>
#include <QMetaType>

struct SourceConfig
{
    enum Type{
        Camera,   // 本地 USB / 内置摄像头
        Rtsp      // 网络摄像头（RTSP 流）
    };
    Type    type        = Camera;   // 默认使用摄像头
    int     cameraIndex = 0;        // type==Camera 时使用：设备索引
    QString rtspUrl;                // type==Rtsp 时使用：rtsp:// 地址

    // 期望分辨率（仅对摄像头有效，RTSP 由源端决定）
    int     width       = 1280;
    int     height      = 720;
};
// 让 Qt 的元对象系统认识 SourceConfig
// 这样它才能在跨线程的 invokeMethod / 信号槽中传递
Q_DECLARE_METATYPE(SourceConfig)