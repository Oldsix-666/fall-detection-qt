#pragma once

#include "core/types.h"
#include "source/iframesource.h"

// ============================================================
// FrameSourceFactory：按配置创建对应的视频源（工厂）
// 把"创建哪种源"的决定集中在一处，
// 调用方（DetectionWorker）不需要知道 CameraSource / RtspSource 的存在
// ============================================================
class FrameSourceFactory
{
public:
    // 返回 IFrameSourcePtr（unique_ptr<IFrameSource>）：所有权移交给调用方
    static IFrameSourcePtr create(const SourceConfig& cfg);
};
