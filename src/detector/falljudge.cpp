#include "detector/falljudge.h"

bool FallJudge::update(const QRectF& box, double area)
{
    // 每次更新先重置状态
    m_lastAspect = 0.f;
    m_lastValid  = false;

    bool thisFrame = false;

    // ========================================================
    // 【条件判断】只有前景面积足够大、且框高度有效时才判定
    // box.height() > 1.0 防止除零
    // ========================================================
    if (area > m_params.minArea && box.height() > 1.0) {
        m_lastValid  = true;

        // 【float(...)】C++ 显式转换（C 风格）
        // C++ 里更推荐 static_cast<float>(...)
        m_lastAspect = float(box.width() / box.height());

        // 宽高比大于阈值 → 疑似摔倒
        thisFrame = (m_lastAspect > m_params.aspectMin);
    }

    // 【push_back】在尾部追加
    m_history.push_back(thisFrame);

    // ========================================================
    // 【size()】返回元素个数，int() 强制转成 int 便于比较
    // 保持窗口大小固定，超出就移除头部
    // ========================================================
    while (int(m_history.size()) > m_params.confirmFrames)
        m_history.pop_front();   // 从头部移除

    // ========================================================
    // 【范围 for 循环】C++11
    // 遍历 m_history 里所有元素，b 是 bool 副本
    // ========================================================
    int hit = 0;
    for (bool b : m_history) if (b) ++hit;

    // 只有窗口内全是 true 才认为确认摔倒
    return hit >= m_params.confirmFrames;
}