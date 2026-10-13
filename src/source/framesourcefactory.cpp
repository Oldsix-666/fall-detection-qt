#include "source/framesourcefactory.h"
#include "source/camerasource.h"
#include "source/rtspsource.h"

#include <memory>   // std::make_unique

// ============================================================
// 【返回类型 IFrameSourcePtr】unique_ptr<IFrameSource>
//   所有权转移：函数内创建的 CameraSource/RtspSource，
//   转成 unique_ptr 后由调用方负责析构
// ============================================================
IFrameSourcePtr FrameSourceFactory::create(const SourceConfig& cfg)
{
    // 【switch】根据枚举值分发
    // 每个 case 直接 return，所以不需要 break
    switch (cfg.type) {

    case SourceConfig::Camera:
        // ====================================================
        // 【std::make_unique<T>(args...)】C++14
        // 等价于 std::unique_ptr<T>(new T(args...))
        // 但更安全（异常安全）且更简洁
        // ====================================================
        return std::make_unique<CameraSource>(cfg.cameraIndex, cfg.width, cfg.height);

    case SourceConfig::Rtsp:
        return std::make_unique<RtspSource>(cfg.rtspUrl);
    }

    // 理论上不会到这里，保险起见返回空
    // 【nullptr】现代 C++ 空指针，替代老的 NULL
    return nullptr;
}
