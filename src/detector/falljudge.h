#pragma once

#include <QRectF>    // Qt 浮点矩形
#include <deque>     // 双端队列，做滑动窗口

// ============================================================
// 摔倒判定器
// 算法：宽高比 + 连续帧确认
// 完全无 Qt 事件循环依赖，方便单元测试
// ============================================================
class FallJudge
{
public:
    // 【嵌套 struct】把参数打包
    // 便于传递和整体替换
    struct Params {
        int   confirmFrames = 10;    // 连续多少帧满足条件才报警
        float aspectMin     = 1.20f; // 宽高比阈值（宽/高）
        int   minArea       = 3000;  // 前景最小面积，滤噪
    };

    // 【= default】默认构造：编译器自动生成（无参数）
    FallJudge() = default;

    // 【explicit】禁止隐式转换
    // 带参构造：初始化 m_params
    explicit FallJudge(const Params& p) : m_params(p) {}

    // ========================================================
    // 【setParams】更新参数，同时清空历史
    // 为什么清空：新旧参数下判据不同，旧数据留着会误判
    // ========================================================
    void setParams(const Params& p) { m_params = p; m_history.clear(); }

    // 【reset】只清历史，不动参数。停止/重启时调用
    void reset() { m_history.clear(); }

    // 每帧调用一次
    bool update(const QRectF& box, double area);

    // ========================================================
    // 【getter 内联在头文件】简单访问器直接 inline，减少函数调用开销
    // const 修饰：不修改成员
    // ========================================================
    float lastAspect() const { return m_lastAspect; }
    bool  lastValid()  const { return m_lastValid;  }

private:
    Params           m_params;

    // 【std::deque<bool>】双端队列
    // 选用 deque 而非 vector：需要在头尾都频繁操作
    std::deque<bool> m_history;

    float            m_lastAspect = 0.f;   // 【0.f】浮点字面量
    bool             m_lastValid  = false;
};