# FallDetectionQt

基于 **Qt Widgets + OpenCV** 的跌倒检测桌面程序。接入本地摄像头或 RTSP 网络摄像头，
在子线程里实时做运动检测，判断画面中的人是否跌倒，并在界面上画框、报警。

## 功能

- **两种视频源**，由工厂统一创建、多态调用：
  - 本地摄像头（USB / 内置，按设备索引）
  - RTSP 网络摄像头（海康、大华等）
- **子线程检测**：采集与检测跑在独立线程，界面不卡顿；帧通过信号槽跨线程回传
- **实时可视化**：人体外接矩形、宽高比数值、FPS；检测到跌倒时叠加红色报警横幅
- **可调判定参数**：连续帧数、宽高比阈值、前景最小面积

## 检测流程

```
视频源 → read() → 降采样到 320x240 → MOG2 背景减除 → 形态学开运算去噪
                                                          ↓
     画框 / 报警横幅 ← FallJudge 判定 ← 找最大轮廓 boundingRect
```

1. **降采样**到 320×240 再做检测，显著降低 CPU 占用
2. **`createBackgroundSubtractorMOG2(500, 16, false)`** 把运动前景从静止背景中分离
3. **形态学开运算**（5×5 椭圆核）去掉零散噪点
4. **找最大轮廓**，坐标按缩放比例映射回原图
5. **`FallJudge` 判定**：宽高比超阈值 + 连续 N 帧确认

## 项目结构

| 路径 | 说明 |
| --- | --- |
| `FallDetectionQt.pro` | qmake 工程文件 |
| `main.cpp` | 入口；显式注册跨线程传递的元类型 `SourceConfig` |
| `MainWindow.ui` | Qt Designer 界面 |
| `src/core/types.h` | 公共类型：`SourceConfig`（视频源配置，`Q_DECLARE_METATYPE`） |
| `src/source/iframesource.h` | 视频源抽象接口（纯虚基类 + `unique_ptr` 别名） |
| `src/source/camerasource.*` | 本地摄像头实现 |
| `src/source/rtspsource.*` | RTSP 流实现 |
| `src/source/framesourcefactory.*` | 工厂：按配置创建对应类型的源 |
| `src/detector/detectionworker.*` | 子线程工作对象：采集 + 检测主循环 |
| `src/detector/falljudge.*` | 跌倒判定算法（零 Qt 依赖，便于单元测试） |
| `src/ui/mainwindow.*` | 主窗口 |
| `src/ui/videowidget.*` | 视频显示控件 |

## 环境要求

| 依赖 | 版本 | 备注 |
| --- | --- | --- |
| Qt | 6.x（Widgets 模块） | |
| OpenCV | **5.0**，`vc16` 构建 | 需要 `opencv_world500` 与 `videoio` |
| 编译器 | MSVC | 需与 OpenCV 的 `vc16` ABI 对应 |

> ⚠️ **OpenCV 5.0 的模块划分变了**：`contourArea` / `boundingRect` 被移到了
> `opencv2/geometry.hpp`，`opencv2/opencv.hpp` 不再包含它。代码里已显式 include。

## 构建

### 1. 改 OpenCV 路径（必做）

`FallDetectionQt.pro` 顶部的 `OPENCV_DIR` 默认写的是作者本机路径：

```qmake
OPENCV_DIR = D:/Tool/Opencv/opencv/build
```

改成你自己的 OpenCV 构建根目录，该目录下应有：

```
include/
x64/vc16/lib/opencv_world500.lib
x64/vc16/bin/opencv_world500.dll
```

### 2. 构建

用 **Qt Creator** 打开 `FallDetectionQt.pro`，选好 Kit，直接构建运行。

或命令行：

```bash
qmake FallDetectionQt.pro
nmake        # MSVC；用 MinGW 的话是 mingw32-make
```

链接完成后，`.pro` 里的 `QMAKE_POST_LINK` 会自动把 `opencv_world500*.dll` 和
`opencv_videoio_*.dll` 拷到 exe 旁边，不用手工配 PATH。

## 使用

1. 选择视频源类型：
   - **摄像头**：填设备索引（`0` 一般是内置摄像头）
   - **RTSP**：填完整地址，格式参考：
     ```
     rtsp://用户名:密码@摄像头IP:554/Streaming/Channels/101
     ```
     > 源码里预填的 `rtsp://admin:password@192.168.1.100:554/Streaming/Channels/101`
     > 只是**海康威视的地址格式示例**，不是真实账号密码，请替换成你自己的。
2. 点「开始」，画面实时刷新；检测到跌倒时，画面底部会出现红色的 `FALL DETECTED` 横幅。

## 跌倒判定参数

在 `src/detector/falljudge.h` 的 `FallJudge::Params` 里调整：

| 参数 | 默认值 | 含义 |
| --- | --- | --- |
| `confirmFrames` | `10` | 连续多少帧满足条件才报警，用于抗瞬时误判 |
| `aspectMin` | `1.20` | 外接矩形 宽/高 的阈值，超过说明人躺倒了 |
| `minArea` | `3000` | 前景最小面积，用于滤掉噪声 |

改参数会自动清空历史记录（新旧参数判据不同，留着旧数据会误判）。

## 已知限制

- 判定只用了**宽高比**，没有姿态估计，因此"弯腰""蹲下""坐下"这类动作容易误报；
  接入人体关键点 / 姿态模型可以明显改善
- 每帧只跟踪**面积最大**的那块运动前景，多人场景下其他人会被忽略
- RTSP 断流后不会自动重连（连续读帧失败约 1 秒即退出），需要手动重新开始
- 检测在 320×240 上做，距离远的弱小目标容易丢失

## 开源协议

[MIT License](LICENSE)
